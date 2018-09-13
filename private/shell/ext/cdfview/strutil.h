//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// string.h 
//
//   String functions used by cdfview that are not in shlwapi.h.
//
//   History:
//
//       5/15/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _STRING_H_

#define _STRING_H_

//
// String function declarations.
//
#ifdef UNICODE
#define StrEql          StrEqlW
#else // UNICODE
#define StrEql          StrEqlA
#endif // UNICODE 

#define StrCpyA     lstrcpyA
#define StrCpyNA    lstrcpynA
#define StrCatA     lstrcatA

#define StrLen      lstrlen            // lstrlen works in W95.
#define StrLenA     lstrlenA
#define StrLenW     lstrlenW

//
// Function prototypes.
//

BOOL StrEqlA(LPCSTR p1, LPCSTR p2);
BOOL StrEqlW(LPCWSTR p1, LPCWSTR p2);

BOOL StrLocallyDisplayable(LPCWSTR pwsz);

#endif // _STRING_H_
