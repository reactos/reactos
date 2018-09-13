/* This file is copied to winrev.h before compiling 'C' source files... */

#define    WINDOWS_30  TRUE

#define    NOMETAFILE 
#define    NOSOUND
#define    NOKANJI	  

#ifdef WINDOWS_30
#define NOLSTRING    TRUE  /* jtf win3 mod */
#else
#define ES_OEMCONVERT 0
#define SS_NOPREFIX  128L   /* 0x80 - don't do "&" character translation */
#endif

