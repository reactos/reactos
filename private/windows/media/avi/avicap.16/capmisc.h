/****************************************************************************
 *
 *   capmisc.h
 * 
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and 
 *    distribute the Sample Files (and/or any modified version) in 
 *    any way you find useful, provided that you agree that 
 *    Microsoft has no warranty obligations or liability for any 
 *    Sample Application Files which are modified. 
 *
 ***************************************************************************/

void statusUpdateFromID (LPCAPSTREAM lpcs, DWORD dwError);
void statusUpdateStatus (LPCAPSTREAM lpcs, LPSTR lpc);
void ErrMsgID (LPCAPSTREAM lpcs, DWORD dwID);
