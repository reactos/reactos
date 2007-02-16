/*
 * CRTinit.c
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * A dummy version of _CRT_INIT for MS compatibility. Programs, or more often
 * dlls, which use the static version of the MSVC run time are supposed to
 * call _CRT_INIT to initialize the run time library in DllMain. This does
 * not appear to be necessary when using crtdll or the dll versions of the
 * MSVC runtime, so the dummy call simply does nothing.
 *
 * This object file is included as a standard in the link process as provided
 * by the appropriate GCC frontend.
 * 
 * To use this object include the object file in your link command:
 * gcc -o foo.exe foo.o CRTinit.o
 *
 */

void
_CRT_INIT ()
{
}
