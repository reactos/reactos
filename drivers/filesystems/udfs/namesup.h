////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef __UDF_NAME_SUP__H__
#define __UDF_NAME_SUP__H__

extern PWCHAR __fastcall UDFDissectName(IN  PWCHAR   Buffer,
                             OUT PUSHORT  Length);

extern BOOLEAN UDFIsNameInExpression(IN PVCB Vcb,
                                     IN PUNICODE_STRING FileName,
                                     IN PUNICODE_STRING PtrSearchPattern,
                                     OUT PBOOLEAN DosOpen,
                                     IN BOOLEAN IgnoreCase,
                                     IN BOOLEAN ContainsWC,
                                     IN BOOLEAN CanBe8dot3,
                                     IN BOOLEAN KeepIntact);

extern BOOLEAN UDFDoesNameContainWildCards(IN PUNICODE_STRING SearchPattern);

extern BOOLEAN __fastcall UDFIsNameValid(IN PUNICODE_STRING SearchPattern,
                              OUT BOOLEAN* StreamOpen,
                              OUT ULONG* SNameIndex);

extern BOOLEAN __fastcall UDFIsMatchAllMask(IN PUNICODE_STRING Name,
                                 OUT BOOLEAN* DosOpen);

extern BOOLEAN __fastcall UDFCanNameBeA8dot3(IN PUNICODE_STRING Name);

#endif //__UDF_NAME_SUP__H__
