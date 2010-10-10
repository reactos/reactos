/*
 * CRT_glob.c
 * This file has no copyright is assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within the package.
 *
 * Include this object file to set _dowildcard to a state that will turn on
 * command line globbing by default. (wildcard.o which goes into libmingw32.a
 * has a default state of off.)
 *
 * To use this object include the object file in your link command:
 * gcc -o foo.exe foo.o CRT_glob.o
 *
 */

int _dowildcard = -1;

