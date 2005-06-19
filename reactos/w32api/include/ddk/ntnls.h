/*
 * ntddmou.h
 *
 * Structures and definitions for NLS data types.
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Alex Ionescu <alex@relsoft.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __NTNLS_H
#define __NTNLS_H

#define MAXIMUM_LEADBYTES 12

typedef struct _CPTABLEINFO 
{
  USHORT  CodePage;
  USHORT  MaximumCharacterSize;
  USHORT  DefaultChar;
  USHORT  UniDefaultChar;
  USHORT  TransDefaultChar;
  USHORT  TransUniDefaultChar;
  USHORT  DBCSCodePage;
  UCHAR  LeadByte[MAXIMUM_LEADBYTES];
  PUSHORT  MultiByteTable;
  PVOID  WideCharTable;
  PUSHORT  DBCSRanges;
  PUSHORT  DBCSOffsets;
} CPTABLEINFO, *PCPTABLEINFO;

typedef struct _NLSTABLEINFO 
{
  CPTABLEINFO  OemTableInfo;
  CPTABLEINFO  AnsiTableInfo;
  PUSHORT  UpperCaseTable;
  PUSHORT  LowerCaseTable;
} NLSTABLEINFO, *PNLSTABLEINFO;

#endif /* __NTNLS_H */
