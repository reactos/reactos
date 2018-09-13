/*----------------------------------------------------------------------------
	%%File: lexint_.h
	%%Unit: fechmap
	%%Contact: jpick

	Header file for internal FarEast lexer modules.
----------------------------------------------------------------------------*/

#ifndef LEXINT__H
#define LEXINT__H

#include <windows.h>
#include <stdio.h>
#include <stddef.h>


// REVIEW:  other common internal lexer defs go here.
//

// Token type
//
typedef unsigned char JTK;

// Two-Byte Character Mode Mask
//
#define grfTwoByte		(JTK) 0x80

// Longest *character* sequence (not escape sequence -- this
// is the length of the longest multi-byte character).
//
#define cchSeqMax		4

// Prototypes/Defines for the format validation module
//
#define grfValidateCharMapping		0x0001
#define grfCountCommonChars			0x0002

void ValidateInit(ICET icetIn, DWORD dwFlags);
void ValidateInitAll(DWORD dwFlags);
void ValidateReset(ICET icetIn);
void ValidateResetAll(void);
int  NValidateUch(ICET icetIn, UCHAR uch, BOOL fEoi);
BOOL FValidateCharCount(ICET icetIn, int *lpcMatch);

#endif     // #ifndef LEXINT__H
