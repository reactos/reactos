/*
 * noglob.c
 *
 * This file defines _CRT_glob to have a value of 0, which will
 * turn off command line globbing. It is compiled into a separate object
 * file which you can add to your link line to turn off globbing like
 * this:
 *
 * gcc -o foo.exe foo.o noglob.o
 *
 * $Revision: 1.1 $
 * $Author: rex $
 * $Date: 1999/01/16 02:11:43 $
 *
 */

int _CRT_glob = 0;

