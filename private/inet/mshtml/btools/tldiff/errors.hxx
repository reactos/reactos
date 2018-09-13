#if !defined(__errors_hxx__)
#define __errors_hxx__

enum errcodes
{
    ERR_OPENINGCUR,
    ERR_OPENINGREF,
    ERR_OPENINGDIFF,
    ERR_OPENINGNEW,
    ERR_OPENINGMIRROR,
    ERR_MISSINGARGS,
    ERR_INVALIDARG,
    ERR_CURFILENAME,
    ERR_REFFILENAME,
    ERR_INIFILENAME,
    ERR_INIFILENOTFOUND,
    ERR_OUTOFMEMORY,
    ERR_DIFFERENCES,        // If you change the last item, you will have to change DisplayError.
};

char* pszErrors[] =
{ 
    "Error opening current file.",                  // ERR_OPENINGCUR
    "Error opening reference file.",                // ERR_OPENINGREF
    "Error opening differences output file.",       // ERR_OPENINGDIFF
    "Error opening additions output file.",         // ERR_OPENINGNEW
    "Error opening mirror file.",                   // ERR_OPENINGMIRROR
    "Invalid number of input arguments.",           // ERR_MISSINGARGS
    "Invalid argument specified.",                  // ERR_INVALIDARG
    "Current output file name not specified.",      // ERR_CURFILENAME
    "Reference output file name not specified.",    // ERR_REFFILENAME
    "Initialization file name not specified.",      // ERR_INIFILENAME
    "INI file not found.",                          // ERR_INIFILENOTFOUND
    "Out of memory",                                // ERR_OUTOFMEMORY
    "Differences exist between the current file and references file.",  // ERR_DIFFERENCES
};

#endif  // !defined(__errors_hxx__)
