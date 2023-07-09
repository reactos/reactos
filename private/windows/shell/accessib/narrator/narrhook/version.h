// NARRHOOK.DLL version.h
#ifdef RC_INVOKED

// Only pull in WINVER.H if we need it.
#ifndef VER_H
#include <winver.h>
#endif/*VER_H*/

#define VER_IS_SET                  // Lets the RC know we're providing version strings
#define VER_FILEDESCRIPTION_STR     "Microsoft Narrator Keyboard and WinEvent hook"
#define VER_INTERNALNAME_STR        "NARRHOOK"
#define VER_ORIGINALFILENAME_STR    "NARRHOOK.DLL"
#define VER_FILETYPE                VFT_DLL
#define VER_FILESUBTYPE             VFT2_UNKNOWN

#endif/* RC_INVOKED */
