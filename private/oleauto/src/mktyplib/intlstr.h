// INTLSTR.H
//   interface to internationalized data
//

#ifdef	__cplusplus
extern "C" {
#endif

extern CHAR * szBanner;		// mktyplib banner
extern CHAR * szUsage;		// usage string
extern CHAR * szAppTitle;	// application title


// strings for error message formatting/display
extern CHAR * rgszErr[];	// array of error strings
extern CHAR * szFmtSuccess;		// success format string
extern CHAR * szFmtErrFileLineCol;	// error format strings
extern CHAR * szFmtWarnFileLineCol;
extern CHAR * szFmtErrOutput;
extern CHAR * szFmtErrImportlib;
extern CHAR * szFmtErrUnknown;
extern CHAR * szFmtErrGeneral;
extern CHAR * szFmtWarnGeneral;

// strings for header file output
extern CHAR * szHeadFile;
extern CHAR * szHeadModule;
extern CHAR * szHeadInter;
extern CHAR * szHeadDispinter;
extern CHAR * szHeadMethods;
extern CHAR * szHeadDispatchable;

#ifdef	__cplusplus
}
#endif
