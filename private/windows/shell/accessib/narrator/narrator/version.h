// NARRATOR.EXE version.h
#ifdef RC_INVOKED

// Only pull in WINVER.H if we need it.
#ifndef VER_H
#include <winver.h>
#endif/*VER_H*/

#define VER_IS_SET                  // Lets the RC know we're providing version strings
#define VER_FILEDESCRIPTION_STR     "Microsoft Narrator"
#define VER_INTERNALNAME_STR        "NARRATOR"
#define VER_ORIGINALFILENAME_STR    "NARRATOR.EXE"
#define VER_FILETYPE                VFT_APP
#define VER_FILESUBTYPE             VFT2_UNKNOWN

#endif/* RC_INVOKED */
