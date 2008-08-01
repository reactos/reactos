/*
 * CRTfmode.c
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Include this object to set _CRT_fmode to a state that will cause
 * _mingw32_init_fmode to leave all file modes in their default state
 * (basically text mode).
 *
 * To use this object include the object file in your link command:
 * gcc -o foo.exe foo.o CRTfmode.o
 *
 */

int _CRT_fmode = 0;
