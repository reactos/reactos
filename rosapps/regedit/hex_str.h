///////////////////////////////////////////////////////////////////////////////
// HEX_STR.H
//  Copyright (c) 1995 by Robert Dickenson
//
#ifndef __HEX_STR_H__
#define __HEX_STR_H__

#ifdef __cplusplus
extern "C" {
#endif


unsigned short Byte2Hex(unsigned char);
unsigned long Word2Hex(unsigned short);
unsigned char HexByte(char*);
unsigned int HexWord(char*);
unsigned long HexLong(char*);
unsigned long HexString(char*);

void ByteBinStr(char*, unsigned char);
void WordBinStr(char*, unsigned short);
unsigned long HexString(char*);


#ifdef __cplusplus
}
#endif

#endif // __HEX_STR_H__
///////////////////////////////////////////////////////////////////////////////
