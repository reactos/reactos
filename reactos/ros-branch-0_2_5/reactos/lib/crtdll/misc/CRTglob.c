/*
 * CRTglob.c
 *
 * This object file defines _CRT_glob to have a value of -1, which will
 * turn on command line globbing by default. If you want to turn off
 * command line globbing include a line
 *
 * int _CRT_glob = 0;
 *
 * in one of your source modules.
 *
 * $Revision: 1.1 $
 * $Author: rex $
 * $Date: 1999/01/16 02:11:43 $
 *
 */

int _CRT_glob = -1;

