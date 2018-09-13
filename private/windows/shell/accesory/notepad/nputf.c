/*
 * nputf.c  - Routines for utf text processing for notepad
 *
 *   Copyright (C) 1998-1999 Microsoft Inc.
 */

#include "precomp.h"


/* IsTextUTF8
 *
 * UTF-8 is the encoding of Unicode based on Internet Society RFC2279
 * ( See http://www.cis.ohio-state.edu/htbin/rfc/rfc2279.html )
 *
 * Basicly:
 * 0000 0000-0000 007F - 0xxxxxxx  (ascii converts to 1 octet!)
 * 0000 0080-0000 07FF - 110xxxxx 10xxxxxx    ( 2 octet format)
 * 0000 0800-0000 FFFF - 1110xxxx 10xxxxxx 10xxxxxx (3 octet format)
 * (this keeps going for 32 bit unicode) 
 * 
 *
 * Return value:  TRUE, if the text is in UTF-8 format.
 *                FALSE, if the text is not in UTF-8 format.
 *                We will also return FALSE is it is only 7-bit ascii, so the right code page
 *                will be used.
 *
 *                Actually for 7 bit ascii, it doesn't matter which code page we use, but
 *                notepad will remember that it is utf-8 and "save" or "save as" will store
 *                the file with a UTF-8 BOM.  Not cool.
 */


INT IsTextUTF8( LPSTR lpstrInputStream, INT iLen )
{
    INT   i;
    DWORD cOctets;  // octets to go in this UTF-8 encoded character
    UCHAR chr;
    BOOL  bAllAscii= TRUE;

    cOctets= 0;
    for( i=0; i < iLen; i++ ) {
        chr= *(lpstrInputStream+i);

        if( (chr&0x80) != 0 ) bAllAscii= FALSE;

        if( cOctets == 0 )  {
            //
            // 7 bit ascii after 7 bit ascii is just fine.  Handle start of encoding case.
            //
            if( chr >= 0x80 ) {  
               //
               // count of the leading 1 bits is the number of characters encoded
               //
               do {
                  chr <<= 1;
                  cOctets++;
               }
               while( (chr&0x80) != 0 );

               cOctets--;                        // count includes this character
               if( cOctets == 0 ) return FALSE;  // must start with 11xxxxxx
            }
        }
        else {
            // non-leading bytes must start as 10xxxxxx
            if( (chr&0xC0) != 0x80 ) {
                return FALSE;
            }
            cOctets--;                           // processed another octet in encoding
        }
    }

    //
    // End of text.  Check for consistency.
    //

    if( cOctets > 0 ) {   // anything left over at the end is an error
        return FALSE;
    }

    if( bAllAscii ) {     // Not utf-8 if all ascii.  Forces caller to use code pages for conversion
        return FALSE;
    }

    return TRUE;
}


/* IsInputTextUnicode
 * Verify if the input stream is in Unicode format.
 *
 * Return value:  TRUE, if the text is in Unicode format.
 *
 * 29 June 1998          
 */


INT IsInputTextUnicode  (LPSTR lpstrInputStream, INT iLen)
{
    INT  iResult= ~0; // turn on IS_TEXT_UNICODE_DBCS_LEADBYTE
    BOOL bUnicode;

    // We would like to check the possibility
    // of IS_TEXT_UNICODE_DBCS_LEADBYTE.
    //

    bUnicode= IsTextUnicode( lpstrInputStream, iLen, &iResult);

    if (bUnicode                                         &&
       ((iResult & IS_TEXT_UNICODE_STATISTICS)    != 0 ) &&
       ((iResult & (~IS_TEXT_UNICODE_STATISTICS)) == 0 )    )
    {
        CPINFO cpiInfo;
        CHAR* pch= (CHAR*)lpstrInputStream;
        INT  cb;

        //
        // If the result depends only upon statistics, check
        // to see if there is a possibility of DBCS.
        // Only do this check if the ansi code page is DBCS
        //

        GetCPInfo( CP_ACP, &cpiInfo);

        if( cpiInfo.MaxCharSize > 1 )
        {
            for( cb=0; cb<iLen; cb++ )
            {
                if( IsDBCSLeadByte(*pch++) )
                {
                    return FALSE;
                }
            }
        }
     }

     return bUnicode;
}
