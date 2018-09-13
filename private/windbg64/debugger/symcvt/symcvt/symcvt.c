/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    symcvt.c

Abstract:

    This module is the shell for the SYMCVT DLL.  The DLL's purpose is
    to convert the symbols for the specified image.  The resulting
    debug data must conform to the CODEVIEW spec.

    Currently this DLL converts COFF symbols and C7/C8 MAPTOSYM SYM files.

Author:

    Wesley A. Witt (wesw) 19-April-1993

Environment:

    Win32, User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "symcvt.h"
#include "cofftocv.h"
#include "symtocv.h"



PUCHAR
ConvertSymbolsForImage(
                       HANDLE      hFile,
                       char *      fname
    )
/*++

Routine Description:

    Calls the appropriate conversion routine based on the file contents.


Arguments:

    hFile         -  file handle for the image (may be NULL)
    fname         -  file name for the image (may not have correct path)


Return Value:

    NULL             - could not convert the symbols
    Valid Pointer    - a pointer to malloc'ed memory that contains the
                       CODEVIEW symbols

--*/
{
    POINTERS   p;
    char       szDrive    [_MAX_DRIVE];
    char       szDir      [_MAX_DIR];
    char       szFname    [_MAX_FNAME];
    char       szExt      [_MAX_EXT];
    char       szSymName  [MAX_PATH];
    PUCHAR     rVal;


    if (!MapInputFile( &p, hFile, fname)) {

        rVal = NULL;

    } else if (CalculateNtImagePointers( &p.iptrs )) {

        //
        // we were able to compute the nt image pointers so this must be
        // a nt PE image.  now we must decide if there are coff symbols
        // if there are then we do the cofftocv conversion.
        //
        // btw, this is where someone would convert some other type of
        // symbols that are in a nt PE image. (party on garth..)
        //

//      if (!COFF_DIR(&p.iptrs)) {
        if (!p.iptrs.numberOfSymbols) {
            rVal = NULL;
        } else {
            ConvertCoffToCv( &p );
            rVal = p.pCvStart.ptr;
        }
        UnMapInputFile( &p );
        if (p.iptrs.rgDebugDir) {
            LocalFree(p.iptrs.rgDebugDir);
        }

    } else {

        UnMapInputFile ( &p );

        _splitpath( fname, szDrive, szDir, szFname, szExt );
        _makepath( szSymName, szDrive, szDir, szFname, "sym" );

        if (!MapInputFile( &p, NULL, szSymName)) {

            rVal = NULL;

        } else {

            //
            // must be a wow/dos app and there is a .sym file so lets to
            // the symtocv conversion
            //

            ConvertSymToCv( &p );
            UnMapInputFile( &p );

            rVal = p.pCvStart.ptr;
        }

    }

    return rVal;
}
