#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#define WIN_SEPARATOR_CHAR '\\'
#define WIN_SEPARATOR_STRING "\\"
#define NIX_SEPARATOR_CHAR '/'
#define NIX_SEPARATOR_STRING "/"

#ifdef WIN32
#define DIR_SEPARATOR_CHAR WIN_SEPARATOR_CHAR
#define DIR_SEPARATOR_STRING WIN_SEPARATOR_STRING
#define BAD_SEPARATOR_CHAR NIX_SEPARATOR_CHAR
#define MKDIR(s) mkdir(s)
#else
#define DIR_SEPARATOR_CHAR NIX_SEPARATOR_CHAR
#define DIR_SEPARATOR_STRING NIX_SEPARATOR_STRING
#define BAD_SEPARATOR_CHAR WIN_SEPARATOR_CHAR
#define MKDIR(s) mkdir(s,0755)
#endif

char*
convert_path(char* origpath)
{
	char* newpath;
	int i;

	newpath=malloc(strlen(origpath)+1);
	strcpy(newpath,origpath);

	i = 0;
	while (newpath[i] != 0)
	{
		if (newpath[i] == BAD_SEPARATOR_CHAR)
		{
			newpath[i] = DIR_SEPARATOR_CHAR;
		}
		i++;
	}
	return(newpath);
}

#define TRANSFER_SIZE      (65536)

int mkdir_p(char* path)
{
	if (chdir(path) == 0)
	{
		return(0);
	}
	if (MKDIR(path) != 0)
	{
		perror("Failed to create directory");
		exit(1);
	}
	if (chdir(path) != 0)
	{
		perror("Failed to change directory");
		exit(1);
	}
	return(0);
}

int main(int argc, char* argv[])
{
	char* path1;
	char* csec;
	char buf[256];

	if (argc != 2)
	{
		fprintf(stderr, "Too many arguments\n");
		exit(1);
	}

	path1 = convert_path(argv[1]);

	if (isalpha(path1[0]) && path1[1] == ':' && path1[2] == DIR_SEPARATOR_CHAR)
	{
		csec = strtok(path1, DIR_SEPARATOR_STRING);
		sprintf(buf, "%s\\", csec);
		chdir(buf);
		csec = strtok(NULL, DIR_SEPARATOR_STRING);
	}
	else if (path1[0] == DIR_SEPARATOR_CHAR)
	{
		chdir(DIR_SEPARATOR_STRING);
		csec = strtok(path1, DIR_SEPARATOR_STRING);
	}
	else
	{
		csec = strtok(path1, DIR_SEPARATOR_STRING);
	}

	while (csec != NULL)
	{
		mkdir_p(csec);
		csec = strtok(NULL, DIR_SEPARATOR_STRING);
	}

	exit(0);
}
