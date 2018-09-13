/***************************************************************
*
*  UCONVERT - Unicode File conversion
*
*
*  Author: Asmus Freytag
*
*  Copyright (C) 1991, Microsoft Corporation
*-----------------------------------------------------
* header file for Uconvert */

#ifdef UNICODE

#define BUFFER_TOO_SMALL                0x0005

#define ISUNICODE_ASCII16               0x0001
#define ISUNICODE_REVERSE_ASCII16       0x0010
#define ISUNICODE_STATISTICS            0x0002
#define ISUNICODE_REVERSE_STATISTICS    0x0020
#define ISUNICODE_CONTROLS              0x0004
#define ISUNICODE_REVERSE_CONTROLS      0x0040

#define ISUNICODE_SIGNATURE             0x0008
#define ISUNICODE_REVERSE_SIGNATURE     0x0080

#define ISUNICODE_ILLEGAL_CHARS         0x0100
#define ISUNICODE_ODD_LENGTH            0x0200

#define ISUNICODE_NULL_BYTES            0x1000

#define ISUNICODE_UNICODE_MASK          0x000F
#define ISUNICODE_REVERSE_MASK          0x00F0
#define ISUNICODE_NOT_UNICODE_MASK      0x0F00
#define ISUNICODE_NOT_ASCII_MASK        0xF000

#define UNICODE_FFFF              0xFFFF
#define REVERSE_BYTE_ORDER_MARK   0xFFFE
#define BYTE_ORDER_MARK           0xFEFF

#define PARAGRAPH_SEPARATOR       0x2029
#define LINE_SEPARATOR            0x2028


#define UNICODE_TAB               0x0009
#define UNICODE_LF                0x000A
#define UNICODE_CR                0x000D
#define UNICODE_SPACE             0x0020
#define UNICODE_CJK_SPACE         0x3000

#define UNICODE_R_TAB             0x0900
#define UNICODE_R_LF              0x0A00
#define UNICODE_R_CR              0x0D00
#define UNICODE_R_SPACE           0x2000
#define UNICODE_R_CJK_SPACE       0x0030  /* Ambiguous - same as ASCII '0' */

#define ASCII_CRLF                0x0A0D


BOOL IsUnicode  (LPTSTR lpBuff, int iSize, LPINT lpiResult);

#endif

