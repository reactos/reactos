// global.h
//

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

// hard-coded colors to be used instead of COLOR_BTNFACE, _BTNTEXT, etc.
//  - hese are for access via MyGetSysColor() - and GetSysBrush()
#define CMP_COLOR_HILITE    25  // RGB(255, 255, 255)
#define CMP_COLOR_LTGRAY    26  // RGB(192, 192, 192) - instead of BtnFace
#define CMP_COLOR_DKGRAY    27  // RGB(128, 128, 128)
#define CMP_COLOR_BLACK     28  // RGB(0, 0, 0) - instead of frame

// - these are for when all you need is a RGB value)
#define CMP_RGB_HILITE      RGB(255, 255, 255)
#define CMP_RGB_LTGRAY      RGB(192, 192, 192)  // instead of BtnFace
#define CMP_RGB_DKGRAY      RGB(128, 128, 128)
#define CMP_RGB_BLACK       RGB(0, 0, 0)        // instead of frame

#define HID_BASE_BUTTON    0x00070000UL        // IDMB and IDMY

extern CBrush*  GetHalftoneBrush();
extern CBrush*  GetSysBrush(UINT nSysColor);
extern void     ResetSysBrushes();
extern COLORREF MyGetSysColor(UINT nSysColor);

// Remove the drive and directory from a file name...
//
CString StripPath(const TCHAR* szFilePath);

// Remove the name part of a file path.  Return just the drive and directory.
//
CString StripName(const TCHAR* szFilePath);

// Remove the name part of a file path.  Return just the drive and directory, and name.
//
CString StripExtension(const TCHAR* szFilePath);

// Get only the extension of a file path.
//
CString GetExtension(const TCHAR* szFilePath);

// Get the name of a file path.
//
CString GetName(const TCHAR* szFilePath);

// Return the path to szFilePath relative to szDirectory.  (E.g. if szFilePath
// is "C:\FOO\BAR\CDR.CAR" and szDirectory is "C:\FOO", then "BAR\CDR.CAR"
// is returned.  This will never use '..'; if szFilePath is not in szDirectory
// or a sub-directory, then szFilePath is returned unchanged.
//
CString GetRelativeName(const TCHAR* szFilePath, const TCHAR* szDirectory = NULL);

void PreTerminateList( CObList* pList );

#endif // __GLOBAL_H__
