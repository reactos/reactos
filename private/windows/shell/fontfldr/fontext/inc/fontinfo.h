/***************************************************************************
 * fontinfo.h - declarations for the font info class
 *
 * This header defines interfaces for the following classes:
 *      OS_2TableClass - holds info from the OS_2 table (not true class)
 *      TTFInfoClass   - holds partial info from the TT file.
 *      FontInfoClass  - hold font info - this is the true public interface
 *
 *  By putting these guys in classes, not only do we ensure they're not
 *  used in some bad fashions, but we can automatically handle memory
 *  release (via destructors)
 *
 * Copyright (C) 1992-93 ElseWare Corporation.  All rights reserved.
 ***************************************************************************/

#ifndef __FONTINFO_H__
#define __FONTINFO_H__

typedef    TCHAR    FQNAME[255];

class far OS_2TableClass {
     public :  OS_2TableClass () {};
     public :    // fields:

        WORD   wVersion;
        short  ixAvgCharWidth;
        WORD   wusWeightClass;
        WORD   wusWidthClass;
        short  ifsType;
        short  iySubXSize;
        short  iySubYSize;
        short  iySubXOffset;
        short  iySubYOffset;
        short  iySupXSize;
        short  iySupYSize;
        short  iySupXOffset;
        short  iySupYOffset;
        short  iyStrikeoutSize;
        short  iyStrikeoutPosition;
        short  isFamilyClass;
        PANOSEBytesClass PANOSE;
        ULONG  ulCharRange[4];
        TCHAR  achVendID[4];
        WORD   wfsSelection;
        WORD   wusFirstCharIndex;
        WORD   wusLastCharIndex;
        WORD   wusTypoAscender;
        WORD   wusTypeoDescender;
        WORD   wwsTypoLineGap;
        WORD   wusWinAscent;
        WORD   wusWinDescent;
};
typedef OS_2TableClass* LPOS_2TABLE;

/* The string block is a dynamically allocated section of the
 * TRUETYPE info block.  We don't have any idea how big these guys
 * can be until we probe and dig into the file
 */

typedef struct _tagStrings {
        LPTSTR     m_lpszCopyright;
        LPTSTR     m_lpszTrademark;
        LPTSTR     m_lpszVersion;
}    InfoStrings_t, FAR* LPInfoStrings;

class far TTFInfoClass {
public :     
            TTFInfoClass () { vClear (); };
                ~TTFInfoClass () { vFree  (); };
        VOID    vFree( ) {  if (m_lpInfoStrings.m_lpszCopyright)
                                delete[] m_lpInfoStrings.m_lpszCopyright;
                            if( m_lpInfoStrings.m_lpszTrademark )
                                delete[] m_lpInfoStrings.m_lpszTrademark;
                            if( m_lpInfoStrings.m_lpszVersion )
                                delete[] m_lpInfoStrings.m_lpszVersion;

                            vClear (); };

        BOOL    bLoadInfo( HDC hDC );
        // BOOL    bGrowTTFInfo (WORD wLen, LPWORD, LPWORD);

private :    // routines

        VOID    vClear( ) { memset (this, 0, sizeof(*this) ); };

public  :    // fields
   InfoStrings_t    m_lpInfoStrings; // The OS2 table we've allocated
};

class FontInfoClass {
    public : // routines

        FontInfoClass    () {  vClear(); };
        VOID    vClear    () {  m_bValid = FALSE; vFree (); };
        VOID    vFree     () {  m_xTTFInfo.vFree(); };

        LPTSTR lpGetTrademark ()
            {    LPTSTR lp = NULL;
                if( m_bValid )
                    lp = m_xTTFInfo.m_lpInfoStrings.m_lpszTrademark;
                return lp; };

        LPTSTR lpGetCopyright ()
            {    LPTSTR lp = NULL;
                if( m_bValid )
                    lp = m_xTTFInfo.m_lpInfoStrings.m_lpszCopyright;
                return lp; };

        void    vGetVersion( UINT id, PTSTR szVersion );
        BOOL    bGetFontInfo( CFontClass* lpFontRec );
        BOOL    bValid( )   { return m_bValid; };

    private    :    // fields
        BOOL            m_bValid;
        TTFInfoClass    m_xTTFInfo;
        OS_2TableClass  m_OS_2Table;
};
#endif



/****************************************************************************
 * $lgb$
 * 1.0     7-Mar-94   eric Initial revision.
 * $lge$
 *
 ****************************************************************************/

