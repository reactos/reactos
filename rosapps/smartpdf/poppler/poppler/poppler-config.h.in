//================================================= -*- mode: c++ -*- ====
//
// poppler-config.h
//
// Copyright 1996-2004 Glyph & Cog, LLC
//
//========================================================================

#ifndef POPPLER_CONFIG_H
#define POPPLER_CONFIG_H

// We duplicate some of the config.h #define's here since they are
// used in some of the header files we install.  The #ifndef/#endif
// around #undef look odd, but it's to silence warnings about
// redefining those symbols.

/* Enable multithreading support. */
#ifndef MULTITHREADING
#undef MULTITHREADED
#endif

/* Full path for the system-wide xpdfrc file. */
#ifndef SYSTEM_XPDFRC
#undef SYSTEM_XPDFRC
#endif

/* Include support for OPI comments. */
#ifndef OPI_SUPPORT
#undef OPI_SUPPORT
#endif

/* Enable word list support. */
#ifndef TEXTOUT_WORD_LIST
#undef TEXTOUT_WORD_LIST
#endif

// Also, there's a couple of preprocessor symbols in the header files
// that are used but never defined: DISABLE_OUTLINE, DEBUG_MEM and

//------------------------------------------------------------------------
// version
//------------------------------------------------------------------------

// xpdf version
#define xpdfVersion         "3.00"
#define xpdfVersionNum      3.00
#define xpdfMajorVersion    3
#define xpdfMinorVersion    0
#define xpdfMajorVersionStr "3"
#define xpdfMinorVersionStr "0"

// supported PDF version
#define supportedPDFVersionStr "1.5"
#define supportedPDFVersionNum 1.5

// copyright notice
#define xpdfCopyright "Copyright 1996-2004 Glyph & Cog, LLC"

// Windows resource file stuff
#define winxpdfVersion "WinXpdf 3.00"
#define xpdfCopyrightAmp "Copyright 1996-2004 Glyph && Cog, LLC"

//------------------------------------------------------------------------
// paper size
//------------------------------------------------------------------------

// default paper size (in points) for PostScript output
#ifdef A4_PAPER
#define defPaperWidth  595    // ISO A4 (210x297 mm)
#define defPaperHeight 842
#else
#define defPaperWidth  612    // American letter (8.5x11")
#define defPaperHeight 792
#endif

//------------------------------------------------------------------------
// config file (xpdfrc) path
//------------------------------------------------------------------------

// user config file name, relative to the user's home directory
#if defined(VMS) || (defined(WIN32) && !defined(__CYGWIN32__))
#define xpdfUserConfigFile "xpdfrc"
#else
#define xpdfUserConfigFile ".xpdfrc"
#endif

// system config file name (set via the configure script)
#ifdef SYSTEM_XPDFRC
#define xpdfSysConfigFile SYSTEM_XPDFRC
#else
// under Windows, we get the directory with the executable and then
// append this file name
#define xpdfSysConfigFile "xpdfrc"
#endif

//------------------------------------------------------------------------
// X-related constants
//------------------------------------------------------------------------

// default maximum size of color cube to allocate
#define defaultRGBCube 5

// number of fonts (combined t1lib, FreeType, X server) to cache
#define xOutFontCacheSize 64

// number of Type 3 fonts to cache
#define xOutT3FontCacheSize 8

//------------------------------------------------------------------------
// popen
//------------------------------------------------------------------------

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define popen _popen
#define pclose _pclose
#endif

#if defined(VMS) || defined(VMCMS) || defined(DOS) || defined(OS2) || defined(__EMX__) || defined(WIN32) || defined(__DJGPP__) || defined(MACOS)
#define POPEN_READ_MODE "rb"
#else
#define POPEN_READ_MODE "r"
#endif

//------------------------------------------------------------------------
// Win32 stuff
//------------------------------------------------------------------------

#ifdef CDECL
#undef CDECL
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define CDECL __cdecl
#else
#define CDECL
#endif

//------------------------------------------------------------------------
// Compiler
//------------------------------------------------------------------------

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define GCC_PRINTF_FORMAT(fmt_index, va_index) \
	__attribute__((__format__(__printf__, fmt_index, va_index)))
#else
#define GCC_PRINTF_FORMAT(fmt_index, va_index)
#endif


#endif /* POPPLER_CONFIG_H */
