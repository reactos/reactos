//========================================================================
//
// SplashErrorCodes.h
//
//========================================================================

#ifndef SPLASHERRORCODES_H
#define SPLASHERRORCODES_H

//------------------------------------------------------------------------

#define splashOk                 0	// no error

#define splashErrNoCurPt         1	// no current point

#define splashErrEmptyPath       2	// zero points in path

#define splashErrBogusPath       3	// only one point in subpath

#define splashErrNoSave	         4	// state stack is empty

#define splashErrOpenFile        5	// couldn't open file

#define splashErrNoGlyph         6	// couldn't get the requested glyph

#define splashErrModeMismatch    7	// invalid combination of color modes

#define splashErrSingularMatrix  8	// matrix is singular

#define splashErrZeroImage       9      // image of 0x0

#endif
