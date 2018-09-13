 /**

Title:

	getbldda.c

Author:

	Greg Franklin (gregfra)

Syntax:

	GETBLDDA dropname

Synopsis:

	Convert a Trident version number (embedded in a	drop
	name) to a date string, then create a batch file to
	set the environment variable _BLDDATE to this string.

	If the drop name is invalid, use the current system
	date (local, not GMT) to construct the date for
	_BLDDATE.

	The date format used is yyyy-mm-dd because this format
	is what the Diamond Disk Layout utility demands for
	use in its INF files.

Sample usage (in a batch file):

	getbldda %_DROPNAME%
	call ~v.bat
	del ~v.bat
	echo Current build date is %_BLDDATE% >> build.log

Compiling instructions:

	Needs Win32 C++ compiler.

	MS C++ compiler syntax: cl getbldda.c -link

	INCLUDE variable points to C:\MSDEV\INCLUDE (or the equivalent)
	LIB variable points to C:\MSDEV\LIB (or the equivalent)

Version history:

	1996-06-18	First version

**/

/* Includes */
#include <windows.h>
#include <stdio.h>

/* Prototypes */
void
GetDateFromForms3Version (
unsigned int	EncodedDate,
PSYSTEMTIME		d
);

void
GetDateFromSystem (
PSYSTEMTIME		d
);


main(int argc, char* argv[])
	{
	char*	pszDropName;		/* Input is Forms^3 drop name				*/
	int		EncodedDate;     	/* Year/month/day encoded as a number   	*/
	SYSTEMTIME	BuildDate;		/* Output date computed from EncodedDate	*/
	FILE*	BATFILE;			/* Output batch file holding SET cmd		*/

	/* Provide help if the user enters nothing or /? at the cmd line */
	if ((argc < 2) || (strcmp (argv[1],"/?") == 0) 
					|| (strcmp (argv[1],"-?") == 0))
		{
		fprintf(stderr, "Syntax: %s <Forms^3 drop name>\n", argv[0]);
		return 1;
		}

	/* Interpret first cmd line argument as a drop name */
	pszDropName = argv[1];

	/* If dropname too short to hold version number, use system date	*/
	if ((pszDropName == NULL) || (strlen(pszDropName) < 2))
		{
		GetDateFromSystem (&BuildDate);
		}

	/* Otherwise, try to compute a date from the drop name */
	else
		{
		/* Grab numeric date from drop name */
		EncodedDate = atoi (&pszDropName[2]);

		/* If grab failed, use the system date */
		if ((EncodedDate == 0) || (strlen(pszDropName) < 6))
			{
			GetDateFromSystem (&BuildDate);
			}
		else
			{
			/* Valid numeric date exists -- decode it */
			GetDateFromForms3Version (EncodedDate, &BuildDate);
			}
		}

	/* Create the batch file */
	if ((BATFILE = fopen("~v.bat", "wt")) == NULL)
		{
		fprintf(stderr, "Cannot open output file.\n");
		return 1;
		}
	fprintf (BATFILE, "SET _BLDDATE=%04u-%02u-%02u\n",
						BuildDate.wYear, BuildDate.wMonth, BuildDate.wDay);
	fclose (BATFILE);

	/* Exit program */
	printf ("Batch file created.\n"
			"Call ~V.BAT to set the value of _BLDDATE.\n");
	return 0;

	} /*end of GetBldDa*/


void
GetDateFromForms3Version (
unsigned int	EncodedDate,
PSYSTEMTIME		d
)

	/* Synopsis: Compute discrete year, month, day from a Forms^3
					version number represented as an encoded date.

		Test cases:

		0101 = 1996-10-01
		0401 = 1997-01-01
		1301 = 1997-10-01
	*/

	{
	int EncodedYearMonth;	/* Yr/mo value derived from encoded date*/

	EncodedYearMonth = EncodedDate / 100;

	/* Year and month are found from 1st 2 digits of 4-digit code */
	(*d).wYear = 1996 + (EncodedYearMonth+8) / 12;

	(*d).wMonth = (9 + EncodedYearMonth) % 12;
	if ((*d).wMonth == 0)
		{
		(*d).wMonth = 12;
		}

	/* Day = last 2 digits of 4-digit code */
	(*d).wDay = EncodedDate % 100;

	}

void
GetDateFromSystem (
PSYSTEMTIME		d
)
	/* Synopsis: Fill in year, month, day values from the system date.
	*/

	{
	/* GetLocalTime() is a Win32 API function in winbase.h. It takes
		the place of _dos_getdate() or getdate() in typical DOS
		programming. */
	GetLocalTime (d);
	}
