///////////////////////////////////////////////////////////////////////////////
// HEX_STR.C
//  Copyright (c) 1995 by Robert Dickenson
//
#include "hex_str.h"


#define LOBYTE(w)   ((unsigned char)(w))
#define HIBYTE(w)   ((unsigned char)((unsigned short)(w) >> 8))


unsigned char AsciiTable[256][3] = {
 "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F",
 "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F",
 "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
 "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F",
 "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F",
 "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
 "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F",
 "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F",
 "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
 "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F",
 "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF",
 "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
 "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF",
 "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF",
 "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
 "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"
};

unsigned char HexTable[24] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 16
};

unsigned long BinaryNibbles[] = {
  (unsigned long)"0000",
  (unsigned long)"0001",
  (unsigned long)"0010",
  (unsigned long)"0011",
  (unsigned long)"0100",
  (unsigned long)"0101",
  (unsigned long)"0110",
  (unsigned long)"0111",
  (unsigned long)"1000",
  (unsigned long)"1001",
  (unsigned long)"1010",
  (unsigned long)"1011",
  (unsigned long)"1100",
  (unsigned long)"1101",
  (unsigned long)"1110",
  (unsigned long)"1111"
};


void ByteBinStr(char* dst, unsigned char num)
{
  *(unsigned long*)dst++ = BinaryNibbles[(num >> 4) & 0x07];
  *(unsigned long*)dst++ = BinaryNibbles[num & 0x07];
}

void WordBinStr(char* dst, unsigned short num)
{
  ByteBinStr(dst, HIBYTE(num));
  ByteBinStr(dst+4, LOBYTE(num));
}

unsigned short Byte2Hex(unsigned char data)
{
  register unsigned short result;

  result  = AsciiTable[data][1] << 8;
  result += AsciiTable[data][0];
  return result;
}

unsigned long Word2Hex(unsigned short data)
{
  register unsigned long result;

  result  = (unsigned long)Byte2Hex(LOBYTE(data)) << 16;
  result += Byte2Hex(HIBYTE(data));
  return result;
}

unsigned char HexByte(char* str)
{
  register unsigned char result;

  result  = HexTable[*str++ - '0'] * 16;
  result += HexTable[*str++ - '0'];
  return result;
}

unsigned int HexWord(char* str)
{
  register unsigned int result;

  result  = HexByte(str) << 8;
  result += HexByte(str+2);
  return result;
}

unsigned long HexLong(char* str)
{
  register unsigned long result;

  result  = HexByte(str++) << 24;
  result += HexByte(str++) << 16;
  result += HexByte(str++) << 8;
  result += HexByte(str);
  return result;
}

unsigned long HexString(char* str)
{
  unsigned long temp = 0;

  while ( *str )
  {
    temp <<= 4;
    switch ( *str++ )
    {
      case '0':  break;
      case '1':  temp += 1;   break;
      case '2':  temp += 2;   break;
      case '3':  temp += 3;   break;
      case '4':  temp += 4;   break;
      case '5':  temp += 5;   break;
      case '6':  temp += 6;   break;
      case '7':  temp += 7;   break;
      case '8':  temp += 8;   break;
      case '9':  temp += 9;   break;
      case 'A':  temp += 10;  break;
      case 'B':  temp += 11;  break;
      case 'C':  temp += 12;  break;
      case 'D':  temp += 13;  break;
      case 'E':  temp += 14;  break;
      case 'F':  temp += 15;  break;
      case 'a':  temp += 10;  break;
      case 'b':  temp += 11;  break;
      case 'c':  temp += 12;  break;
      case 'd':  temp += 13;  break;
      case 'e':  temp += 14;  break;
      case 'f':  temp += 15;  break;
      case 'X':  temp = 0;  break;
      case 'x':  temp = 0;  break;
      default:   return temp;
    }
  }
  return temp;
}

///////////////////////////////////////////////////////////////////////////////
