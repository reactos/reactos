#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#if USE_CRT
#define unDName __unDName
#endif

#include "undname.h"


//  Prototype for usage fxn.
void usage();


//	Simple 'main' wrapper for the undecorator.  This 'main' takes arguments,
//	each of which is considered to be a decorated name.  It prints the string
//	representing the decorated name, and then the corresponding undecorated
//	name as returned by the undecorator.
//
//	If the first argument to the program is a number (decimal), then that
//	number is used as the value for the undecorator's attribute flags.  See
//	the file 'undname.h' for details of these values.
//
//	Each remaining argument will be passed to the undecorator, even if they
//	are numbers.
//
//	Exit codes are :-
//		0	The program worked.  This does not mean that the name was
//			successfully undecorated
//		1	The program falied.  Some internal error occurred in the undecorator,
//			most likely an out of memory error.  Should never happen.

int	__cdecl	main ( int argc, pchar_t* argv )
{
	int				errCode	= 0;
	unsigned short	flags	= 0;

	// If user has UNDNAME env flags set, use them as defaults
	char *szTempFlag = getenv("UNDNAME");
	if(szTempFlag != NULL)
		flags = strtoul(szTempFlag, NULL, 0);

	// Usage message, if necessary
	if( argc < 2 || strstr(argv[1], "-?") != NULL || strstr(argv[1], "/?") != NULL )
	{
		errCode = 1;
		usage();
	}

	else
	{
		if	(( argc > 2 ) && ( *argv[ 1 ] >= '0' ) && ( *argv[ 1 ] <= '9' )) 
		{
			flags	= (unsigned short)strtoul ( argv[ 1 ], NULL, 0);
			argc--, argv++;
		}

		for	( int i = 1; i < argc; i++ )
		{
			printf ( "Undecoration of :- \"%s\"\n", argv[ i ]);

			pchar_t	uName	= unDName	(	0,
											argv[ i ],
											0,
											&malloc,
											&free,
											flags
										);

			if	( uName )
				printf ( "is :- \"%s\"\n\n", uName );
			else
			{
				printf ( "Internal Error in Undecorator\n" );

				errCode++;

			}	// End of IF else
		}	// End of FOR
	} // End of ELSE

	return	errCode;

}	// End of PROGRAM

void usage()
{
	printf("\nUsage-\n\nundname [flags] fname [fname...]\n\nwhere flags are the following OR'd together:\n\n"
		"0x0001 Remove leading underscores from MS extended keywords\n"
		"0x0002 Disable expansion of MS extended keywords\n"
		"0x0004 Disable expansion of return type for primary declaration\n"
		"0x0008 Disable expansion of the declaration model\n"
		"0x0010 Disable expansion of the declaration language specifier\n"
  		"0x0020 Disable expansion of MS keywords on the 'this' type for primary\n"
  		"       declaration (NYI)\n"
  		"0x0040 Disable expansion of CV modifiers on the 'this' type for primary\n"
  		"       declaration (NYI)\n"
		"0x0060 Disable all modifiers on the 'this' type\n"
		"0x0080 Disable expansion of access specifiers for members\n"
		"0x0100 Disable expansion of 'throw-signatures' for functions and pointers to\n"
		"       functions\n"
		"0x0200 Disable expansion of 'static' or 'virtual'ness of members\n"
		"0x0400 Disable expansion of MS model for UDT returns\n"
		"0x0800 Undecorate 32-bit decorated names\n"
		"0x1000 Crack only the name for primary declaration\n" 
		"       return just [scope::]name.  Does expand template params\n"
		"0x2000 Input is just a type encoding; compose an abstract declarator\n");
}											
