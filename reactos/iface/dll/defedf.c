/* $Id: defedf.c,v 1.2 1999/07/12 21:02:06 ea Exp $
 * 
 * reactos/iface/dll/defedf.c
 *
 * ReactOS Operating System
 *
 * Convert a *.def file for a PE image into an *.edf file,
 * to build the PE image with a clean exports table.
 *
 * Written by EA (19990703)
 *
 */
#include <stdio.h>
#include <stdlib.h>
#ifndef MAX_PATH
#define MAX_PATH _MAX_PATH
#endif
#include <string.h>

#define INPUT_BUFFER_SIZE 1024

static const char * SUFFIX_DEF = ".def";
static const char * SUFFIX_EDF = ".edf";


int
Usage ( const char * ImageName )
{
	fprintf(
		stderr,
		"Usage: %s def_file [edf_file]\n",
		ImageName
		);
	return EXIT_SUCCESS;
}


char *
AddSuffix (
	const char	* Prototype,
	const char	* Suffix
	)
{
	char	NewName [MAX_PATH];
	char	* SuffixStart;

	if (!Prototype) return NULL;
	strcpy( NewName, Prototype );
	SuffixStart = 
		NewName
		+ strlen(NewName)
		- strlen(Suffix);
	if (strcmp(SuffixStart,Suffix))
	{
		strcat(NewName, Suffix);
	}
	return strdup(NewName);
}


char *
MakeEdfName (
	const char	* NameDef,
	const char	* NameEdf
	)
{
	if (NULL == NameEdf)
	{
		char	NewName [MAX_PATH];
		char	* Dot;

		strcpy( NewName, NameDef );
		Dot = strrchr( NewName, '.');
		if (0 == strcmp(Dot, SUFFIX_DEF))
		{
			*Dot = '\0';
		}
		return AddSuffix( NewName, SUFFIX_EDF );
	}
	return AddSuffix( NameEdf, SUFFIX_EDF );
}


typedef
enum 
{
	LineLibrary,
	LineExports,
	LineImports,
	LineSymbol,
	LineComment,
	LineEmpty
	
} PARSING_EXIT_CODE;


PARSING_EXIT_CODE
ParseInput (
	char * InputBuffer,
	char * CleanName
	)
{
	char	Buffer [MAX_PATH];
	char	* r;
	char	* w;

	r = strrchr( InputBuffer, '\n' );
	if (r) *r = '\0';
#ifdef DEBUG
printf("ParseInput(%s)\n",InputBuffer);
#endif
	if (0 == strlen(InputBuffer))
	{
#ifdef DEBUG
printf("LineEmpty\n");
#endif
		return LineEmpty;
	}
	
	for (	r = InputBuffer, w = Buffer;
		*r && (*r != ' ') && (*r != '\t');
		++r
		);
	if (*r == ';') 
	{
		strcpy( InputBuffer, r );
#ifdef DEBUG
printf("LineComment\n");
#endif
		return LineComment;
	}
	r = strchr( InputBuffer, '=' );
	if (r)
	{
		printf( "Fatal error: can not process DEF files with aliases!\n");
		exit(EXIT_FAILURE);
	}
	r = strchr( InputBuffer, '@' );
	if (r)
	{
		strcpy( CleanName, InputBuffer );
		r = strchr( CleanName, '@' );
		*r = '\0';
#ifdef DEBUG
printf("LineSymbol: \"%s\"=\"%s\"\n",InputBuffer,CleanName);
#endif
		return LineSymbol;
	}
	/* can not recognize it; copy it verbatim */
#ifdef DEBUG
printf("LineComment\n");
#endif
	return LineComment;
}


int
DefEdf (
	const char	* ImageName,
	const char	* Def,
	const char	* Edf
	)
{
	FILE	* fDef;
	FILE	* fEdf;
	char	InputBuffer [INPUT_BUFFER_SIZE];

	printf(
		"%s --> %s\n",
		Def,
		Edf
		);
	fDef = fopen( Def, "r" );
	if (!fDef)
	{
		fprintf(
			stderr,
			"%s: could not open \"%s\"\n",
			ImageName,
			Def
			);
		return EXIT_FAILURE;
	}
	fEdf = fopen( Edf, "w" );
	if (!fEdf)
	{
		fprintf(
			stderr,
			"%s: could not create \"%s\"\n",
			ImageName,
			Edf
			);
		return EXIT_FAILURE;
	}
	while ( fgets( InputBuffer, sizeof InputBuffer, fDef ) )
	{
		char CleanName [MAX_PATH];

		switch (ParseInput(InputBuffer,CleanName))
		{
		case LineLibrary:
			fprintf(fEdf,"%s\n",InputBuffer);
			break;
			
		case LineExports:
			fprintf(fEdf,"EXPORTS\n");
			break;
			
		case LineImports:
			fprintf(fEdf,"IMPORTS\n");
			break;			

		case LineSymbol:
			fprintf(
				fEdf,
				"%s=%s\n",
				CleanName,
				InputBuffer
				);
			break;

		case LineComment:
			fprintf(
				fEdf,
				"%s\n",
				InputBuffer
				);
			break;
			
		case LineEmpty:
			fprintf(
				fEdf,
				"\n"
				);
			break;
		}
	}
	fclose(fDef);
	fclose(fEdf);
	return EXIT_SUCCESS;
}


int
main(
	int	argc,
	char	* argv []
	)
{
	char	* NameDef;
	char	* NameEdf;
	int	rv;


	if ((argc != 2) && (argc != 3))
	{
		return Usage(argv[0]);
	}
	NameDef = AddSuffix(
			argv [1],
			SUFFIX_DEF
			);
	if (!NameDef)
	{
		fprintf(
			stderr,
			"%s: can not build the def_file name\n",
			argv [0]
			);
		return EXIT_FAILURE;
	}
	NameEdf = MakeEdfName(
			NameDef,
			argv [2]
			);
	if (!NameEdf)
	{
		fprintf(
			stderr,
			"%s: can not build the edf_file name\n",
			argv [0]
			);
		free(NameDef);
		return EXIT_FAILURE;
	}
	rv = DefEdf(
		argv [0],
		NameDef,
		NameEdf
		);
	free(NameDef);
	free(NameEdf);
	return rv;
}

/* EOF */
