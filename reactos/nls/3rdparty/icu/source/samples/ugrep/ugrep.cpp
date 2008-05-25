/**************************************************************************
*
*   Copyright (C) 2002, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
***************************************************************************
*/

//
//   ugrep  - an ICU sample program illustrating the use of ICU Regular Expressions.
//
//            The use of the ICU Regex API all occurs within the main()
//            function.  The rest of the code deals with with opening files,
//            encoding conversions, printing results, etc.
//
//            This is not a full-featured grep program.  The command line options
//            have been kept to a minimum to avoid complicating the sample code.
//



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/regex.h"
#include "unicode/ucnv.h"
#include "unicode/uclean.h"


//
//  The following variables contain paramters that may be set from the command line.
//
const char *pattern = NULL;     // The regular expression
int        firstFileNum;        //  argv index of the first file name
UBool      displayFileName = FALSE;
UBool      displayLineNum  = FALSE;


//
//  Info regarding the file currently being processed
//
const char *fileName;      
int         fileLen;              // Length, in UTF-16 Code Units.  

UChar      *ucharBuf = 0;         // Buffer, holds converted file.  (Simple minded program, always reads
                                  //   the whole file at once.

char       *charBuf = 0;          // Buffer, for original, unconverted file data.


//
//  Info regarding the line currently being processed
//
int      lineStart;     // Index of first char of the current line in the file buffer
int      lineEnd;       // Index of char following the new line sequence for the current line
int      lineNum;

//
//  Converter, used on output to convert Unicode data back to char *
//             so that it will display in non-Unicode terminal windows.
//
UConverter  *outConverter = 0;

//
//  Function forward declarations
//
void processOptions(int argc, const char **argv);
void nextLine(int start);
void printMatch();
void printUsage();
void readFile(const char *name);



//------------------------------------------------------------------------------------------
//
//   main          for ugrep
//
//           Structurally, all use of the ICU Regular Expression API is in main(),
//           and all of the supporting stuff necessary to make a running program, but
//           not directly related to regular expressions, is factored out into these other
//           functions.
//
//------------------------------------------------------------------------------------------
int main(int argc, const char** argv) {
    UBool     matchFound = FALSE;

    //
    //  Process the commmand line options.
    //
    processOptions(argc, argv);

    //
    // Create a RegexPattern object from the user supplied pattern string.
    //
    UErrorCode status = U_ZERO_ERROR;   // All ICU operations report success or failure
                                        //   in a status variable.

    UParseError    parseErr;            // In the event of a syntax error in the regex pattern,
                                        //   this struct will contain the position of the
                                        //   error.

    RegexPattern  *rePat = RegexPattern::compile(pattern, parseErr, status);
                                        // Note that C++ is doing an automatic conversion
                                        //  of the (char *) pattern to a temporary
                                        //  UnicodeString object.
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep:  error in pattern: \"%s\" at position %d\n",
            u_errorName(status), parseErr.offset);
        exit(-1);
    }

    //
    // Create a RegexMatcher from the newly created pattern.
    //
    UnicodeString empty;
    RegexMatcher *matcher = rePat->matcher(empty, status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep:  error in creating RegexMatcher: \"%s\"\n",
            u_errorName(status));
        exit(-1);
    }

    //
    // Loop, processing each of the input files.
    //
    for (int fileNum=firstFileNum; fileNum < argc; fileNum++) {
        readFile(argv[fileNum]);

        //
        //  Loop through the lines of a file, trying to match the regex pattern on each.
        //
        for (nextLine(0); lineStart<fileLen; nextLine(lineEnd)) {
            UnicodeString s(FALSE, ucharBuf+lineStart, lineEnd-lineStart);
            matcher->reset(s);
            if (matcher->find()) {
                matchFound = TRUE;
                printMatch();
            }
        }
    }

    //
    //  Clean up
    //
    delete matcher;
    delete rePat;
    free(ucharBuf);
    free(charBuf);
    ucnv_close(outConverter);
    
    u_cleanup();       // shut down ICU, release any cached data it owns.

    return matchFound? 0: 1;
}



//------------------------------------------------------------------------------------------
//
//   doOptions          Run through the command line options, and set
//                      the global variables accordingly.
//
//                      exit without returning if an error occured and
//                      ugrep should not proceed further.
//
//------------------------------------------------------------------------------------------
void processOptions(int argc, const char **argv) {
    int            optInd;
    UBool          doUsage   = FALSE;
    UBool          doVersion = FALSE;
    const char    *arg;


    for(optInd = 1; optInd < argc; ++optInd) {
        arg = argv[optInd];
        
        /* version info */
        if(strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
            doVersion = TRUE;
        }
        /* usage info */
        else if(strcmp(arg, "--help") == 0) {
            doUsage = TRUE;
        }
        else if(strcmp(arg, "-n") == 0 || strcmp(arg, "--line-number") == 0) {
            displayLineNum = TRUE;
        }
        /* POSIX.1 says all arguments after -- are not options */
        else if(strcmp(arg, "--") == 0) {
            /* skip the -- */
            ++optInd;
            break;
        }
        /* unrecognized option */
        else if(strncmp(arg, "-", strlen("-")) == 0) {
            printf("ugrep: invalid option -- %s\n", arg+1);
            doUsage = TRUE;
        }
        /* done with options */
        else {
            break;
        }
    }

    if (doUsage) {
        printUsage();
        exit(0);
    }

    if (doVersion) {
        printf("ugrep version 0.01\n");
        if (optInd == argc) {
            exit(0);
        }
    }

    int  remainingArgs = argc-optInd;     // pattern file ...
    if (remainingArgs < 2) {
        fprintf(stderr, "ugrep:  files or pattern are missing.\n");
        printUsage();
        exit(1);
    }

    if (remainingArgs > 2) {
        // More than one file to be processed.   Display file names with match output.
        displayFileName = TRUE;
    }

    pattern      = argv[optInd];
    firstFileNum = optInd+1;
}

//------------------------------------------------------------------------------------------
//
//   printUsage
//
//------------------------------------------------------------------------------------------
void printUsage() {
    printf("ugrep [options] pattern file...\n"
        "     -V or --version     display version information\n"
        "     --help              display this help and exit\n"
        "     --                  stop further option processing\n"
        "-n,  --line-number       Prefix each line of output with the line number within its input file.\n"
        );
    exit(0);
}

//------------------------------------------------------------------------------------------
//
//    readFile          Read a file into memory, and convert it to Unicode.
//
//                      Since this is just a demo program, take the simple minded approach
//                      of always reading the whole file at once.  No intelligent buffering
//                      is done.
//
//------------------------------------------------------------------------------------------
void readFile(const char *name) {

    //
    //  Initialize global file variables
    //
    fileName = name;
    fileLen  = 0;      // zero length prevents processing in case of errors.


    //
    //  Open the file and determine its size.
    //
    FILE *file = fopen(name, "rb");
    if (file == 0 ) {
        fprintf(stderr, "ugrep: Could not open file \"%s\"\n", fileName);
        return;
    }
    fseek(file, 0, SEEK_END);
    int rawFileLen = ftell(file);
    fseek(file, 0, SEEK_SET);
    

    //
    //   Read in the file
    //
    charBuf    = (char *)realloc(charBuf, rawFileLen+1);   // Need error checking...
    int t = fread(charBuf, 1, rawFileLen, file);
    if (t != rawFileLen)  {
        fprintf(stderr, "Error reading file \"%s\"\n", fileName);
        return;
    }
    charBuf[rawFileLen]=0;
    fclose(file);

    //
    // Look for a Unicode Signature (BOM) in the data
    //
    int32_t        signatureLength;
    const char *   charDataStart = charBuf;
    UErrorCode     status        = U_ZERO_ERROR;
    const char*    encoding      = ucnv_detectUnicodeSignature(
                           charDataStart, rawFileLen, &signatureLength, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep: ICU Error \"%s\" from ucnv_detectUnicodeSignature()\n",
            u_errorName(status));
        return;
    }
    if(encoding!=NULL ){
        charDataStart  += signatureLength;
        rawFileLen     -= signatureLength;
    }

    //
    // Open a converter to take the file to UTF-16
    //
    UConverter* conv;
    conv = ucnv_open(encoding, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep: ICU Error \"%s\" from ucnv_open()\n", u_errorName(status));
        return;
    }

    //
    // Convert the file data to UChar.
    //  Preflight first to determine required buffer size.
    //
    uint32_t destCap = ucnv_toUChars(conv,
                       NULL,           //  dest,
                       0,              //  destCapacity,
                       charDataStart,
                       rawFileLen,
                       &status);
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        fprintf(stderr, "ugrep: ucnv_toUChars: ICU Error \"%s\"\n", u_errorName(status));
        return;
    };
    
    status = U_ZERO_ERROR;
    ucharBuf = (UChar *)realloc(ucharBuf, (destCap+1) * sizeof(UChar));
    ucnv_toUChars(conv,
        ucharBuf,           //  dest,
        destCap+1,
        charDataStart,
        rawFileLen,
        &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep: ucnv_toUChars: ICU Error \"%s\"\n", u_errorName(status));
        return;
    };
    ucnv_close(conv);
    
    //
    //  Successful conversion.  Set the global size variables so that
    //     the rest of the processing will proceed for this file.
    //
    fileLen = destCap;
}
    
    
    


//------------------------------------------------------------------------------------------
//
//   nextLine           Advance the line index variables, starting at the
//                      specified position in the input file buffer, by
//                      scanning forwrd until the next end-of-line.
//
//                      Need to take into account all of the possible Unicode
//                      line ending sequences.
//
//------------------------------------------------------------------------------------------
void nextLine(int  startPos) {
    if (startPos == 0) {
        lineNum = 0;
    } else {
        lineNum++;
    }
    lineStart = lineEnd = startPos;

    for (;;) {
        if (lineEnd >= fileLen) {
            return;
        }
        UChar c = ucharBuf[lineEnd];
        lineEnd++;
        if (c == 0x0a   ||       // Line Feed
            c == 0x0c   ||       // Form Feed
            c == 0x0d   ||       // Carriage Return
            c == 0x85   ||       // Next Line
            c == 0x2028 ||       // Line Separator
            c == 0x2029)         // Paragraph separator
        { 
            break;
        }
    }

    // Check for CR/LF sequence, and advance over the LF if we're in the middle of one.
    if (lineEnd < fileLen           &&
        ucharBuf[lineEnd-1] == 0x0d &&
        ucharBuf[lineEnd]   == 0x0a) 
    {
        lineEnd++;
    }
}


//------------------------------------------------------------------------------------------
//
//   printMatch         Called when a matching line has been located.
//                      Print out the line from the file with the match, after
//                         converting it back to the default code page.
//
//------------------------------------------------------------------------------------------
void printMatch() {
    char                buf[2000];
    UErrorCode         status       = U_ZERO_ERROR;

    // If we haven't already created a converter for output, do it now.
    if (outConverter == 0) {
        outConverter = ucnv_open(NULL, &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "ugrep:  Error opening default converter: \"%s\"\n",
                u_errorName(status));
            exit(-1);
        }
    };

    // Convert the line to be printed back to the default 8 bit code page.
    //   If the line is too long for our buffer, just truncate it.
    ucnv_fromUChars(outConverter,
                    buf,                   // destination buffer for conversion
                    sizeof(buf),           // capacity of destination buffer
                    &ucharBuf[lineStart],   // Input to conversion
                    lineEnd-lineStart,     // number of UChars to convert
                    &status);
    buf[sizeof(buf)-1] = 0;                // Add null for use in case of too long lines.
                                           // The converter null-terminates its output unless
                                           //   the buffer completely fills.
   
    if (displayFileName) {
        printf("%s:", fileName);
    }
    if (displayLineNum) {
        printf("%d:", lineNum);
    }
    printf("%s", buf);
}
    
