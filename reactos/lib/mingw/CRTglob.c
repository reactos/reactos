/*
 * CRTglob.c
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Include this object file to set _CRT_glob to a state that will
 * turn on command line globbing by default.  NOTE: _CRT_glob has a default
 * state of on.  Specify CRT_noglob.o to turn off globbing by default.
 *
 * To use this object include the object file in your link command:
 * gcc -o foo.exe foo.o CRTglob.o
 *
 */

int _CRT_glob = -1;
