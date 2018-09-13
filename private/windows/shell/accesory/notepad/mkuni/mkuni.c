/****************************************************************************

    PROGRAM: MkUni.c

    PURPOSE: Creates a text file with unicode characters

    FUNCTIONS:

****************************************************************************/

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include "mkuni.h"


#define REVERSE    0
#define LINE_SIZE  1000

#define ASCIIEOL TEXT("\r\n")
#define UNILINESEP 0x2028
#define UNIPARASEP 0x2029

struct __range {
	int	low;
	int	high;
	LPTSTR	pDes;
} range[] = {
            {0x20,  0x7f,   TEXT("ANSI") },
            {0xa0,  0xff,   TEXT("Latin") },
            {0x100, 0x17f,  TEXT("European Latin") },
            {0x180, 0x1f0,  TEXT("Extended Latin") },
            {0x250, 0x2a8,  TEXT("Standard Phonetic") },
            {0x2b0, 0x2e9,  TEXT("Modifier Letters") },
            {0x300, 0x341,  TEXT("Generic Diacritical") },
            {0x370, 0x3f5,  TEXT("Greek") },
            {0x400, 0x486,  TEXT("Cyrillic") },
            {0x490, 0x4cc,  TEXT("Extended Cyrillic") },
            {0x5b0, 0x5f5,  TEXT("Hebrew") },
            {0x0600,0x06F9, TEXT("Arabic") },
            {0x0900,0x0970, TEXT("Devanagari") },
            {0x0E00,0x0E5B, TEXT("Thai") },
            {0x1000,0x104C, TEXT("Tibetan") },
            {0x10A0,0x10FB, TEXT("Georgian") },
            {0x20a0,0x20aa, TEXT("Currency Symbols") },
            {0x2100,0x2138, TEXT("Letterlike Symbols") },
            {0x2153,0x2182, TEXT("Number Forms") },
            {0x2190,0x21ea, TEXT("Arrows") },
            {0x2200,0x22f1, TEXT("Math Operators") },
            {0x2500,0x257F, TEXT("Form and Chart Components") },
            {0x25A0,0x25EE, TEXT("Geometric Shapes") },
            {0x2600,0x266F, TEXT("Miscellaneous Dingbats") },
            {0x3000,0x303F, TEXT("CJK Symbols and Punctuations") },            
            {0x3040,0x309E, TEXT("Hiragana") },
            {0x3100,0x312C, TEXT("Bopomofo") },
            {0x3131,0x318E, TEXT("Hangul Elements") },
            {0,     0,      TEXT("terminating entry") },
            };

/****************************************************************************

    FUNCTION: putu(FILE*pf, TCHAR c)

    PURPOSE: writes a character to the file.
             (Reverses the order of leadbytes if the flag is set)

****************************************************************************/

void
putu(FILE*pf, TCHAR c)
{
    TCHAR chr=c;

    if( REVERSE )
        chr= ( c<<8 ) + ( ( c>>8 ) &0xFF);


	fwrite((void*)&chr, 1, sizeof(TCHAR), pf);
}


/****************************************************************************

    FUNCTION: putust(FILE*pf, LPTSTR pc)

    PURPOSE: writes a string to the file.

****************************************************************************/

void
putust(FILE*pf, LPTSTR pc)
{
	while (*pc)
		putu(pf, *pc++);
}


/****************************************************************************

    FUNCTION: main(int, char**)

    PURPOSE: write sample unicode file

****************************************************************************/

int _cdecl main(int argc, char**argv)
{
    struct __range*pr = range;
    int	    i;
    FILE    *pf;
    FILE    *pfo;
    char    lpstrLine[LINE_SIZE];

    if(!(pf = fopen("unicode.txt", "wb")))
        return FALSE;

    // Task1: Write all the unicode ranges and all the characters 
    // in those ranges to the output file.
    putu(pf, (TCHAR)0xfeff);
    while (pr->low != 0) {
    	putust(pf, TEXT("<<< "));
    	putust(pf, pr->pDes);
    	putust(pf, TEXT(" >>>"));
    	putust(pf, ASCIIEOL );
    	for (i=pr->low ; i<=pr->high ; i++)
    	    putu(pf, (TCHAR)i);
    	putust(pf, ASCIIEOL);
    	pr++;
    }

    putust(pf, TEXT("Unicode Line separator here ->"));
    putu(pf, UNILINESEP );
    putust(pf, TEXT("<- Unicode line separator"));
    putust( pf, ASCIIEOL );
    
    putust(pf, TEXT("Unicode Paragraph separator here ->"));
    putu(pf, UNIPARASEP );
    putust(pf, TEXT("<- Unicode paragraph separator"));
    putust( pf, ASCIIEOL );

    fclose( pf );

    // Task2: Write all the characters codes and information
    // on each character code to an output file.
    if (!(pf = fopen( "names2.txt", "r" )))
        return FALSE;

    if (!(pfo = fopen("unicodes.txt", "wb")))
        return FALSE;

    // The first character should be 0xFEFF in the file, 
    // indicating that it's an unicode file.
    putu( pfo, (TCHAR)0xfeff);

    // Read the input file (names2.txt) which has information
    // on every unicode character.
    do
    {
    WCHAR wLineBuffer[LINE_SIZE];
    int i, num;

        if (!memset(lpstrLine, 0, LINE_SIZE))
        {
            _tprintf(TEXT("Something wrong - failed in Memset!!\n") );
            break;
        }
       
        // fgets returns NULL on eof or on an error condition
        if( fgets( lpstrLine, LINE_SIZE, pf) == NULL )
        {
            if (!feof(pf))
               _tprintf(TEXT("Error occured while reading names2.txt.\n") );

            break;
        }

        i = 0;
        
        // Find the first newline (if there is any) and replace it by \0.
        while((lpstrLine[i]!= '\n') && (lpstrLine[i]!='\r') && (lpstrLine[i]!='\0'))
        {
            i++;
        }

        lpstrLine[i]= '\0';
       
        // If the line has the character code (for which info is given)
        // grab and "display" that.
        num= -1;
        sscanf( lpstrLine, "%x", &num);

        if( num != -1 )
        {
            putu( pfo, (TCHAR) num );
            putust( pfo, TEXT(": ") );
        }
        else
        {
            putust( pfo,TEXT("   ") );
        }
        // Convert it to the world of unicodes.
        if (MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, lpstrLine, -1, 
                        wLineBuffer, LINE_SIZE ))
            putust(pfo, wLineBuffer);

        putust( pfo, ASCIIEOL );

    }
    while( TRUE );

    fclose( pfo );
    fclose( pf );

    return 1;
}
