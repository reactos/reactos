/*
 ******************************************************************************
 *
 *   Copyright (C) 2000-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 ******************************************************************************
 *   file name:  ushape.c
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2000jun29
 *   created by: Markus W. Scherer
 *
 *   Arabic letter shaping implemented by Ayman Roshdy
 */

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/ushape.h"
#include "cmemory.h"
#include "putilimp.h"
#include "ustr_imp.h"
#include "ubidi_props.h"

#if UTF_SIZE<16
    /*
     * This implementation assumes that the internal encoding is UTF-16
     * or UTF-32, not UTF-8.
     * The main assumption is that the Arabic characters and their
     * presentation forms each fit into a single UChar.
     * With UTF-8, they occupy 2 or 3 bytes, and more than the ASCII
     * characters.
     */
#   error This implementation assumes UTF-16 or UTF-32 (check UTF_SIZE)
#endif

/*
 * ### TODO in general for letter shaping:
 * - the letter shaping code is UTF-16-unaware; needs update
 *   + especially invertBuffer()?!
 * - needs to handle the "Arabic Tail" that is used in some legacy codepages
 *   as a glyph fragment of wide-glyph letters
 *   + IBM Unicode conversion tables map it to U+200B (ZWSP)
 *   + IBM Egypt has proposed to encode the tail in Unicode among Arabic Presentation Forms
 */

/* definitions for Arabic letter shaping ------------------------------------ */

#define IRRELEVANT 4
#define LAMTYPE    16
#define ALEFTYPE   32
#define LINKR      1
#define LINKL      2
#define APRESENT   8
#define SHADDA     64
#define CSHADDA    128
#define COMBINE    (SHADDA+CSHADDA)


static const UChar IrrelevantPos[] = {
    0x0, 0x2, 0x4, 0x6,
    0x8, 0xA, 0xC, 0xE,
};

static const UChar convertLamAlef[] =
{
/*FEF5*/    0x0622,
/*FEF6*/    0x0622,
/*FEF7*/    0x0623,
/*FEF8*/    0x0623,
/*FEF9*/    0x0625,
/*FEFA*/    0x0625,
/*FEFB*/    0x0627,
/*FEFC*/    0x0627
};

static const UChar araLink[178]=
{
  1           + 32 + 256 * 0x11,/*0x0622*/
  1           + 32 + 256 * 0x13,/*0x0623*/
  1                + 256 * 0x15,/*0x0624*/
  1           + 32 + 256 * 0x17,/*0x0625*/
  1 + 2            + 256 * 0x19,/*0x0626*/
  1           + 32 + 256 * 0x1D,/*0x0627*/
  1 + 2            + 256 * 0x1F,/*0x0628*/
  1                + 256 * 0x23,/*0x0629*/
  1 + 2            + 256 * 0x25,/*0x062A*/
  1 + 2            + 256 * 0x29,/*0x062B*/
  1 + 2            + 256 * 0x2D,/*0x062C*/
  1 + 2            + 256 * 0x31,/*0x062D*/
  1 + 2            + 256 * 0x35,/*0x062E*/
  1                + 256 * 0x39,/*0x062F*/
  1                + 256 * 0x3B,/*0x0630*/
  1                + 256 * 0x3D,/*0x0631*/
  1                + 256 * 0x3F,/*0x0632*/
  1 + 2            + 256 * 0x41,/*0x0633*/
  1 + 2            + 256 * 0x45,/*0x0634*/
  1 + 2            + 256 * 0x49,/*0x0635*/
  1 + 2            + 256 * 0x4D,/*0x0636*/
  1 + 2            + 256 * 0x51,/*0x0637*/
  1 + 2            + 256 * 0x55,/*0x0638*/
  1 + 2            + 256 * 0x59,/*0x0639*/
  1 + 2            + 256 * 0x5D,/*0x063A*/
  0, 0, 0, 0, 0,                /*0x063B-0x063F*/
  1 + 2,                        /*0x0640*/
  1 + 2            + 256 * 0x61,/*0x0641*/
  1 + 2            + 256 * 0x65,/*0x0642*/
  1 + 2            + 256 * 0x69,/*0x0643*/
  1 + 2       + 16 + 256 * 0x6D,/*0x0644*/
  1 + 2            + 256 * 0x71,/*0x0645*/
  1 + 2            + 256 * 0x75,/*0x0646*/
  1 + 2            + 256 * 0x79,/*0x0647*/
  1                + 256 * 0x7D,/*0x0648*/
  1                + 256 * 0x7F,/*0x0649*/
  1 + 2            + 256 * 0x81,/*0x064A*/
         4         + 256 * 1,   /*0x064B*/
         4 + 128   + 256 * 1,   /*0x064C*/
         4 + 128   + 256 * 1,   /*0x064D*/
         4 + 128   + 256 * 1,   /*0x064E*/
         4 + 128   + 256 * 1,   /*0x064F*/
         4 + 128   + 256 * 1,   /*0x0650*/
         4 + 64    + 256 * 3,   /*0x0651*/
         4         + 256 * 1,   /*0x0652*/
         4         + 256 * 7,   /*0x0653*/
         4         + 256 * 8,   /*0x0654*/
         4         + 256 * 8,   /*0x0655*/ 
         4         + 256 * 1,   /*0x0656*/
  0, 0, 0, 0, 0,                /*0x0657-0x065B*/
  1                + 256 * 0x85,/*0x065C*/
  1                + 256 * 0x87,/*0x065D*/
  1                + 256 * 0x89,/*0x065E*/
  1                + 256 * 0x8B,/*0x065F*/
  0, 0, 0, 0, 0,                /*0x0660-0x0664*/
  0, 0, 0, 0, 0,                /*0x0665-0x0669*/
  0, 0, 0, 0, 0, 0,             /*0x066A-0x066F*/
         4         + 256 * 6,   /*0x0670*/
  1        + 8     + 256 * 0x00,/*0x0671*/
  1            + 32,            /*0x0672*/
  1            + 32,            /*0x0673*/
  0,                            /*0x0674*/
  1            + 32,            /*0x0675*/
  1, 1,                         /*0x0676-0x0677*/
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2, /*0x0678-0x067D*/
  1+2+8+256 * 0x06, 1+2, 1+2, 1+2, 1+2, 1+2, /*0x067E-0x0683*/
  1+2, 1+2, 1+2+8+256 * 0x2A, 1+2,           /*0x0684-0x0687*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*0x0688-0x0691*/
  1, 1, 1, 1, 1, 1, 1+8+256 * 0x3A, 1,       /*0x0692-0x0699*/
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2, /*0x069A-0x06A3*/
  1+2, 1+2, 1+2, 1+2,           /*0x069A-0x06A3*/
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2+8+256 * 0x3E, /*0x06A4-0x06AD*/
  1+2, 1+2, 1+2, 1+2,           /*0x06A4-0x06AD*/
  1+2, 1+2+8+256 * 0x42, 1+2, 1+2, 1+2, 1+2, /*0x06AE-0x06B7*/
  1+2, 1+2, 1+2, 1+2,           /*0x06AE-0x06B7*/
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2, /*0x06B8-0x06BF*/
  1+2, 1+2,                     /*0x06B8-0x06BF*/
  1,                            /*0x06C0*/
  1+2,                          /*0x06C1*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*0x06C2-0x06CB*/
  1+2+8+256 * 0xAC,             /*0x06CC*/
  1,                            /*0x06CD*/
  1+2, 1+2, 1+2, 1+2,           /*0x06CE-0x06D1*/
  1, 1                          /*0x06D2-0x06D3*/
};

static const UChar presALink[] = {
/***********0*****1*****2*****3*****4*****5*****6*****7*****8*****9*****A*****B*****C*****D*****E*****F*/
/*FB5*/    0,    1,    0,    0,    0,    0,    0,    1,    2,1 + 2,    0,    0,    0,    0,    0,    0,
/*FB6*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FB7*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    2,1 + 2,    0,    0,
/*FB8*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    0,    0,    0,    1,
/*FB9*/    2,1 + 2,    0,    1,    2,1 + 2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FBA*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FBB*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FBC*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FBD*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FBE*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FBF*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    2,1 + 2,
/*FC0*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FC1*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FC2*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FC3*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FC4*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/*FC5*/    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    4,    4,
/*FC6*/    4,    4,    4
};

static const UChar presBLink[]=
{
/***********0*****1*****2*****3*****4*****5*****6*****7*****8*****9*****A*****B*****C*****D*****E*****F*/
/*FE7*/1 + 2,1 + 2,1 + 2,    0,1 + 2,    0,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2, 
/*FE8*/    0,    0,    1,    0,    1,    0,    1,    0,    1,    0,    1,    2,1 + 2,    0,    1,    0,
/*FE9*/    1,    2,1 + 2,    0,    1,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,
/*FEA*/1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    0,    1,    0,    1,    0,  
/*FEB*/    1,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,
/*FEC*/1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,
/*FED*/1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,
/*FEE*/1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    0,
/*FEF*/    1,    0,    1,    2,1 + 2,    0,    1,    0,    1,    0,    1,    0,    1,    0,    0,    0
};

static const UChar convertFBto06[] =
{
/***********0******1******2******3******4******5******6******7******8******9******A******B******C******D******E******F***/
/*FB5*/   0x671, 0x671,     0,     0,     0,     0, 0x07E, 0x07E, 0x07E, 0x07E,     0,     0,     0,     0,     0,     0,
/*FB6*/       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
/*FB7*/       0,     0,     0,     0,     0,     0,     0,     0,     0,     0, 0x686, 0x686, 0x686, 0x686,     0,     0,
/*FB8*/       0,     0,     0,     0,     0,     0,     0,     0,     0,     0, 0x698, 0x698,     0,     0, 0x6A9, 0x6A9,
/*FB9*/   0x6A9, 0x6A9, 0x6AF, 0x6AF, 0x6AF, 0x6AF,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
/*FBA*/       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
/*FBB*/       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
/*FBC*/       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
/*FBD*/       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
/*FBE*/       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
/*FBF*/       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0, 0x6CC, 0x6CC, 0x6CC, 0x6CC
};

static const UChar convertFEto06[] =
{
/***********0******1******2******3******4******5******6******7******8******9******A******B******C******D******E******F***/
/*FE7*/   0x64B, 0x64B, 0x64C, 0x64C, 0x64D, 0x64D, 0x64E, 0x64E, 0x64F, 0x64F, 0x650, 0x650, 0x651, 0x651, 0x652, 0x652,
/*FE8*/   0x621, 0x622, 0x622, 0x623, 0x623, 0x624, 0x624, 0x625, 0x625, 0x626, 0x626, 0x626, 0x626, 0x627, 0x627, 0x628,
/*FE9*/   0x628, 0x628, 0x628, 0x629, 0x629, 0x62A, 0x62A, 0x62A, 0x62A, 0x62B, 0x62B, 0x62B, 0x62B, 0x62C, 0x62C, 0x62C,
/*FEA*/   0x62C, 0x62D, 0x62D, 0x62D, 0x62D, 0x62E, 0x62E, 0x62E, 0x62E, 0x62F, 0x62F, 0x630, 0x630, 0x631, 0x631, 0x632,
/*FEB*/   0x632, 0x633, 0x633, 0x633, 0x633, 0x634, 0x634, 0x634, 0x634, 0x635, 0x635, 0x635, 0x635, 0x636, 0x636, 0x636,
/*FEC*/   0x636, 0x637, 0x637, 0x637, 0x637, 0x638, 0x638, 0x638, 0x638, 0x639, 0x639, 0x639, 0x639, 0x63A, 0x63A, 0x63A,
/*FED*/   0x63A, 0x641, 0x641, 0x641, 0x641, 0x642, 0x642, 0x642, 0x642, 0x643, 0x643, 0x643, 0x643, 0x644, 0x644, 0x644,
/*FEE*/   0x644, 0x645, 0x645, 0x645, 0x645, 0x646, 0x646, 0x646, 0x646, 0x647, 0x647, 0x647, 0x647, 0x648, 0x648, 0x649,
/*FEF*/   0x649, 0x64A, 0x64A, 0x64A, 0x64A, 0x65C, 0x65C, 0x65D, 0x65D, 0x65E, 0x65E, 0x65F, 0x65F
};

static const UChar shapeTable[4][4][4]=
{
  { {0,0,0,0}, {0,0,0,0}, {0,1,0,3}, {0,1,0,1} },
  { {0,0,2,2}, {0,0,1,2}, {0,1,1,2}, {0,1,1,3} },
  { {0,0,0,0}, {0,0,0,0}, {0,1,0,3}, {0,1,0,3} },
  { {0,0,1,2}, {0,0,1,2}, {0,1,1,2}, {0,1,1,3} }
};

/*
 * This function shapes European digits to Arabic-Indic digits
 * in-place, writing over the input characters.
 * Since we know that we are only looking for BMP code points,
 * we can safely just work with code units (again, at least UTF-16).
 */
static void
_shapeToArabicDigitsWithContext(UChar *s, int32_t length,
                                UChar digitBase,
                                UBool isLogical, UBool lastStrongWasAL) {
    const UBiDiProps *bdp;
    UErrorCode errorCode;

    int32_t i;
    UChar c;

    errorCode=U_ZERO_ERROR;
    bdp=ubidi_getSingleton(&errorCode);
    if(U_FAILURE(errorCode)) {
        return;
    }

    digitBase-=0x30;

    /* the iteration direction depends on the type of input */
    if(isLogical) {
        for(i=0; i<length; ++i) {
            c=s[i];
            switch(ubidi_getClass(bdp, c)) {
            case U_LEFT_TO_RIGHT: /* L */
            case U_RIGHT_TO_LEFT: /* R */
                lastStrongWasAL=FALSE;
                break;
            case U_RIGHT_TO_LEFT_ARABIC: /* AL */
                lastStrongWasAL=TRUE;
                break;
            case U_EUROPEAN_NUMBER: /* EN */
                if(lastStrongWasAL && (uint32_t)(c-0x30)<10) {
                    s[i]=(UChar)(digitBase+c); /* digitBase+(c-0x30) - digitBase was modified above */
                }
                break;
            default :
                break;
            }
        }
    } else {
        for(i=length; i>0; /* pre-decrement in the body */) {
            c=s[--i];
            switch(ubidi_getClass(bdp, c)) {
            case U_LEFT_TO_RIGHT: /* L */
            case U_RIGHT_TO_LEFT: /* R */
                lastStrongWasAL=FALSE;
                break;
            case U_RIGHT_TO_LEFT_ARABIC: /* AL */
                lastStrongWasAL=TRUE;
                break;
            case U_EUROPEAN_NUMBER: /* EN */
                if(lastStrongWasAL && (uint32_t)(c-0x30)<10) {
                    s[i]=(UChar)(digitBase+c); /* digitBase+(c-0x30) - digitBase was modified above */
                }
                break;
            default :
                break;
            }
        }
    }
}

/*
 *Name     : invertBuffer
 *Function : This function inverts the buffer, it's used
 *           in case the user specifies the buffer to be
 *           U_SHAPE_TEXT_DIRECTION_LOGICAL
 */
static void
invertBuffer(UChar *buffer,int32_t size,uint32_t options,int32_t lowlimit,int32_t highlimit) {
    UChar temp;
    int32_t i=0,j=0;
    for(i=lowlimit,j=size-highlimit-1;i<j;i++,j--) {
        temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }
}

/*
 *Name     : changeLamAlef
 *Function : Converts the Alef characters into an equivalent
 *           LamAlef location in the 0x06xx Range, this is an
 *           intermediate stage in the operation of the program
 *           later it'll be converted into the 0xFExx LamAlefs
 *           in the shaping function.
 */
static U_INLINE UChar
changeLamAlef(UChar ch) {
    switch(ch) {
    case 0x0622 :
        return 0x065C;
    case 0x0623 :
        return 0x065D;
    case 0x0625 :
        return 0x065E;
    case 0x0627 :
        return 0x065F;
    }
    return 0;
}

/*
 *Name     : getLink
 *Function : Resolves the link between the characters as
 *           Arabic characters have four forms :
 *           Isolated, Initial, Middle and Final Form
 */
static UChar
getLink(UChar ch) {
    if(ch >= 0x0622 && ch <= 0x06D3) {
        return(araLink[ch-0x0622]);
    } else if(ch == 0x200D) {
        return(3);
    } else if(ch >= 0x206D && ch <= 0x206F) {
        return(4);
    } else if(ch >= 0xFB50 && ch <= 0xFC62) {
        return(presALink[ch-0xFB50]);
    } else if(ch >= 0xFE70 && ch <= 0xFEFC) {
        return(presBLink[ch-0xFE70]);
    } else {
        return(0);
    }
}

/*
 *Name     : countSpaces
 *Function : Counts the number of spaces
 *           at each end of the logical buffer
 */
static void
countSpaces(UChar *dest,int32_t size,uint32_t options,int32_t *spacesCountl,int32_t *spacesCountr) {
    int32_t i = 0;
    int32_t countl = 0,countr = 0;
    while(dest[i] == 0x0020) {
       countl++;
       i++;
    }
    while(dest[size-1] == 0x0020) {
       countr++;
       size--;
    }
    *spacesCountl = countl;
    *spacesCountr = countr;
}

/*
 *Name     : isTashkeelChar
 *Function : Returns 1 for Tashkeel characters else return 0
 */
static U_INLINE int32_t
isTashkeelChar(UChar ch) {
    return (int32_t)( ch>=0x064B && ch<= 0x0652 );
}

/*
 *Name     : isAlefChar
 *Function : Returns 1 for Alef characters else return 0
 */
static U_INLINE int32_t
isAlefChar(UChar ch) {
    return (int32_t)( (ch==0x0622)||(ch==0x0623)||(ch==0x0625)||(ch==0x0627) );
}

/*
 *Name     : isLamAlefChar
 *Function : Returns 1 for LamAlef characters else return 0
 */
static U_INLINE int32_t
isLamAlefChar(UChar ch) {
    return (int32_t)( (ch>=0xFEF5)&&(ch<=0xFEFC) );
}

/*
 *Name     : calculateSize
 *Function : This function calculates the destSize to be used in preflighting
 *           when the destSize is equal to 0
 */
static int32_t
calculateSize(const UChar *source, int32_t sourceLength,
              int32_t destSize,uint32_t options) {
    int32_t i = 0;
    destSize = sourceLength;
    switch(options&U_SHAPE_LETTERS_MASK) {
    case U_SHAPE_LETTERS_SHAPE :
    case U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED:
        if((options&U_SHAPE_TEXT_DIRECTION_MASK)==U_SHAPE_TEXT_DIRECTION_VISUAL_LTR) {
            for(i=0;i<sourceLength;i++) {
                if( (isAlefChar(source[i]))&&(source[i+1]==0x0644) ) {
                    destSize--;
                }
            }
        } else if((options&U_SHAPE_TEXT_DIRECTION_MASK)==U_SHAPE_TEXT_DIRECTION_LOGICAL) {
            for(i=0;i<sourceLength;i++) {
                if( (isAlefChar(source[i+1]))&&(source[i]==0x0644) ) {
                    destSize--;
                }
            }
        }
        break;

    case U_SHAPE_LETTERS_UNSHAPE :
        for(i=0;i<sourceLength;i++) {
            if( isLamAlefChar(source[i]) ) {
                destSize++;
            }
        }
        break;

    default :
        /* will never occur because of validity checks at the begin of u_shapeArabic */
        break;
    }

    return destSize;
}

/*
 *Name     : removeLamAlefSpaces
 *Function : The shapeUnicode function converts Lam + Alef into LamAlef + space,
 *           this function removes the spaces behind the LamAlefs according to
 *           the options the user specifies, the spaces are removed to the end
 *           of the buffer, or shrink the buffer ab=nd remove spaces for good
 *           or leave the buffer as it is LamAlef + space.
 */
static int32_t
removeLamAlefSpaces(UChar *dest, int32_t sourceLength,
                    int32_t destSize,
                    uint32_t options,
                    UErrorCode *pErrorCode) {

    int32_t i = 0, j = 0;
    int32_t count = 0;
    UChar *tempbuffer=NULL;

    switch(options&U_SHAPE_LENGTH_MASK) {
    case U_SHAPE_LENGTH_GROW_SHRINK :
        tempbuffer = (UChar *)uprv_malloc((sourceLength+1)*U_SIZEOF_UCHAR);
        /* Test for NULL */
        if(tempbuffer == NULL) {
            *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

        uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);

        i = j = 0;
        while(i < sourceLength) {
            if(dest[i] == 0xFFFF) {
                j--;
                count++;
            }
            else
                tempbuffer[j] = dest[i];
            i++;
            j++;
        }

        while(count >= 0) {
            tempbuffer[i] = 0x0000;
            i--;
            count--;
        }

        uprv_memcpy(dest, tempbuffer, sourceLength*U_SIZEOF_UCHAR);
        destSize = u_strlen(dest);
        break;

    case U_SHAPE_LENGTH_FIXED_SPACES_NEAR :
        /* Lam+Alef is already shaped into LamAlef + FFFF */
        i = 0;
        while(i < sourceLength) {
            if(dest[i] == 0xFFFF)
                dest[i] = 0x0020;
            i++;
        }
        destSize = sourceLength;
        break;

    case U_SHAPE_LENGTH_FIXED_SPACES_AT_BEGINNING :
        tempbuffer = (UChar *)uprv_malloc((sourceLength+1)*U_SIZEOF_UCHAR);
        
        /* Test for NULL */
        if(tempbuffer == NULL) {
            *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

        uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);

        i = j = sourceLength;
        while(i >= 0) {
            if(dest[i] == 0xFFFF) {
                j++;
                count++;
            }
            else
                tempbuffer[j] = dest[i];
            i--;
            j--;
        }
        for(i=0;i<count;i++)
            tempbuffer[i] = 0x0020;

        uprv_memcpy(dest, tempbuffer, sourceLength*U_SIZEOF_UCHAR);
        destSize = sourceLength;
        break;

    case U_SHAPE_LENGTH_FIXED_SPACES_AT_END :
        tempbuffer = (UChar *)uprv_malloc((sourceLength+1)*U_SIZEOF_UCHAR);

        /* Test for NULL */
        if(tempbuffer == NULL) {
            *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

        uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);

        i = j = 0;
        while(i < sourceLength) {
            if(dest[i] == 0xFFFF) {
                j--;
                count++;
            }
            else
                tempbuffer[j] = dest[i];
            i++;
            j++;
        }

        while(count >= 0) {
            tempbuffer[i] = 0x0020;
            i--;
            count--;
        }

        uprv_memcpy(dest,tempbuffer, sourceLength*U_SIZEOF_UCHAR);
        destSize = sourceLength;
        break;

    default :
        /* will not occur */
        break;
    }

    if(tempbuffer)
        uprv_free(tempbuffer);

    return destSize;
}

/*
 *Name     : expandLamAlef
 *Function : LamAlef needs special handling as the LamAlef is
 *           one character while expanding it will give two
 *           characters Lam + Alef, so we need to expand the LamAlef
 *           in near or far spaces according to the options the user
 *           specifies or increase the buffer size.
 *           If there are no spaces to expand the LamAlef, an error
 *           will be set to U_NO_SPACE_AVAILABLE as defined in utypes.h
 */
static int32_t
expandLamAlef(UChar *dest, int32_t sourceLength,
              int32_t destSize,uint32_t options,
              UErrorCode *pErrorCode) {

    int32_t      i = 0,j = 0;
    int32_t      countl = 0;
    int32_t      countr = 0;
    int32_t  inpsize = sourceLength;
    UChar    lamalefChar;
    UChar    *tempbuffer=NULL;

    switch(options&U_SHAPE_LENGTH_MASK) {

    case U_SHAPE_LENGTH_GROW_SHRINK :
        destSize = calculateSize(dest,sourceLength,destSize,options);
        tempbuffer = (UChar *)uprv_malloc((destSize+1)*U_SIZEOF_UCHAR);

        /* Test for NULL */
        if(tempbuffer == NULL) {
            *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

        uprv_memset(tempbuffer, 0, (destSize+1)*U_SIZEOF_UCHAR);

        i = j = 0;
        while(i < destSize && j < destSize) {
            if( isLamAlefChar(dest[i]) ) {
                tempbuffer[j] = convertLamAlef[ dest[i] - 0xFEF5 ];
                tempbuffer[j+1] = 0x0644;
                j++;
            }
            else
                tempbuffer[j] = dest[i];
            i++;
            j++;
        }

        uprv_memcpy(dest, tempbuffer, destSize*U_SIZEOF_UCHAR);
        break;

    case U_SHAPE_LENGTH_FIXED_SPACES_NEAR :
        for(i = 0;i<sourceLength;i++) {
            if((dest[i] == 0x0020) && isLamAlefChar(dest[i+1])) {
                lamalefChar = dest[i+1];
                dest[i+1] = 0x0644;
                dest[i] = convertLamAlef[ lamalefChar - 0xFEF5 ];
            }
            else
                if((dest[i] != 0x0020) && isLamAlefChar(dest[i+1])) {
                    *pErrorCode=U_NO_SPACE_AVAILABLE;
                }
        }
        destSize = sourceLength;
        break;

    case U_SHAPE_LENGTH_FIXED_SPACES_AT_BEGINNING :
        tempbuffer = (UChar *)uprv_malloc((sourceLength+1)*U_SIZEOF_UCHAR);

        /* Test for NULL */
        if(tempbuffer == NULL) {
            *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

        uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);

        i = 0;
        while(dest[i] == 0x0020) {
            countl++;
            i++;
        }

        i = j = sourceLength-1;
        while(i >= 0 && j >= 0) {
             if( countl>0 && isLamAlefChar(dest[i]) ) {
                 tempbuffer[j] = 0x0644;
                 tempbuffer[j-1] = convertLamAlef[ dest[i] - 0xFEF5 ];
                 j--;
                 countl--;
             }
             else {
                 if( countl == 0 && isLamAlefChar(dest[i]) )
                     *pErrorCode=U_NO_SPACE_AVAILABLE;
                 tempbuffer[j] = dest[i];
             }
             i--;
             j--;
        }

        uprv_memcpy(dest, tempbuffer, sourceLength*U_SIZEOF_UCHAR);
        destSize = sourceLength;
        break;

        case U_SHAPE_LENGTH_FIXED_SPACES_AT_END :
            /* LamAlef expansion below is done from right to left to make sure that we consume
             * the spaces with the LamAlefs as they appear in the visual buffer from right to left
             */
            tempbuffer = (UChar *)uprv_malloc((sourceLength+1)*U_SIZEOF_UCHAR);

            /* Test for NULL */
            if(tempbuffer == NULL) {
                *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }

            uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);

            while(dest[inpsize-1] == 0x0020) {
                countr++;
                inpsize--;
            }

            i = sourceLength - countr - 1;
            j = sourceLength - 1;

            while(i >= 0 && j >= 0) {
                if( countr>0 && isLamAlefChar(dest[i]) ) {
                    tempbuffer[j] = 0x0644;
                    tempbuffer[j-1] = convertLamAlef[ dest[i] - 0xFEF5 ];
                    j--;
                    countr--;
                }
                else {
                    if( countr == 0 && isLamAlefChar(dest[i]) )
                        *pErrorCode=U_NO_SPACE_AVAILABLE;
                    tempbuffer[j] = dest[i];
                }
                i--;
                j--;
            }

            if(countr > 0) {
                uprv_memmove(tempbuffer, tempbuffer+countr, sourceLength*U_SIZEOF_UCHAR);
                if(u_strlen(tempbuffer) < sourceLength) {
                    for(i=sourceLength-1;i>=sourceLength-countr;i--) {
                        tempbuffer[i] = 0x0020;
                    }
                }
            }

            uprv_memcpy(dest, tempbuffer, sourceLength*U_SIZEOF_UCHAR);

            destSize = sourceLength;
            break;

    default :
        /* will never occur because of validity checks */
        break;
    }

    if(tempbuffer)
        uprv_free(tempbuffer);

    return destSize;
}

/*
 *Name     : shapeUnicode
 *Function : Converts an Arabic Unicode buffer in 06xx Range into a shaped
 *           arabic Unicode buffer in FExx Range
 */
static int32_t
shapeUnicode(UChar *dest, int32_t sourceLength,
             int32_t destSize,uint32_t options,
             UErrorCode *pErrorCode,
             int tashkeelFlag) {

    int32_t          i, iend;
    int32_t          step;
    int32_t          lastPos,Nx, Nw;
    unsigned int     Shape;
    int32_t          lamalef_found = 0;
    UChar            prevLink = 0, lastLink = 0, currLink, nextLink = 0;
    UChar            wLamalef;

    /*
     * Converts the input buffer from FExx Range into 06xx Range
     * to make sure that all characters are in the 06xx range
     * even the lamalef is converted to the special region in
     * the 06xx range
     */
    if ((options & U_SHAPE_PRESERVE_PRESENTATION_MASK)  == U_SHAPE_PRESERVE_PRESENTATION_NOOP) {
        for (i = 0; i < sourceLength; i++) {
            UChar inputChar  = dest[i];
            if ( (inputChar >= 0xFB50) && (inputChar <= 0xFBFF)) {
                UChar c = convertFBto06 [ (inputChar - 0xFB50) ];
                if (c != 0)
                    dest[i] = c;
            } else if ( (inputChar >= 0xFE70) && (inputChar <= 0xFEFC)) {
                dest[i] = convertFEto06 [ (inputChar - 0xFE70) ] ;
            } else {
                dest[i] = inputChar ;
            }
        }
    }

    /* sets the index to the end of the buffer, together with the step point to -1 */
    i = sourceLength - 1;
    iend = -1;
    step = -1;

    /*
     * This function resolves the link between the characters .
     * Arabic characters have four forms :
     * Isolated Form, Initial Form, Middle Form and Final Form
     */
    currLink = getLink(dest[i]);

    lastPos = i;
    Nx = -2, Nw = 0;

    while (i != iend) {
        /* If high byte of currLink > 0 then more than one shape */
        if ((currLink & 0xFF00) > 0 || (getLink(dest[i]) & IRRELEVANT) != 0) {
            Nw = i + step;
            while (Nx < 0) {         /* we need to know about next char */
                if(Nw == iend) {
                    nextLink = 0;
                    Nx = 3000;
                } else {
                    nextLink = getLink(dest[Nw]);
                    if((nextLink & IRRELEVANT) == 0) {
                        Nx = Nw;
                    } else {
                        Nw = Nw + step;
                    }
                }
            }

            if ( ((currLink & ALEFTYPE) > 0)  &&  ((lastLink & LAMTYPE) > 0) ) {
                lamalef_found = 1;
                wLamalef = changeLamAlef(dest[i]); /*get from 0x065C-0x065f */
                if ( wLamalef != 0) {
                    dest[i] = 0xFFFF;            /* The default case is to drop the Alef and replace */
                    dest[lastPos] =wLamalef;     /* it by 0xFFFF which is the last character in the  */
                    i=lastPos;                   /* unicode private use area, this is done to make   */
                }                                /* sure that removeLamAlefSpaces() handles only the */
                lastLink = prevLink;             /* spaces generated during lamalef generation.      */
                currLink = getLink(wLamalef);    /* 0xFFFF is added here and is replaced by spaces   */
            }                                    /* in removeLamAlefSpaces()                         */
            /*  
             * get the proper shape according to link ability of neighbors
             * and of character; depends on the order of the shapes
             * (isolated, initial, middle, final) in the compatibility area
             */
            Shape = shapeTable[nextLink & (LINKR + LINKL)]
                              [lastLink & (LINKR + LINKL)]
                              [currLink & (LINKR + LINKL)]; 

            if ((currLink & (LINKR+LINKL)) == 1) {
                Shape &= 1;
            } else if(isTashkeelChar(dest[i])) {
                if( (lastLink & LINKL) && (nextLink & LINKR) && (tashkeelFlag == 1) &&
                     dest[i] != 0x064C && dest[i] != 0x064D )
                {
                    Shape = 1; 
                    if( (nextLink&ALEFTYPE) == ALEFTYPE && (lastLink&LAMTYPE) == LAMTYPE ) {
                        Shape = 0; 
                    }
                }
                else {
                    Shape = 0;
                }
            }
            if ((dest[i] ^ 0x0600) < 0x100) {
                if(isTashkeelChar(dest[i]))
                    dest[i] =  0xFE70 + IrrelevantPos[(dest[i] - 0x064B)] + Shape;
                else if ((currLink & APRESENT) > 0)
                    dest[i] = (UChar)(0xFB50 + (currLink >> 8) + Shape);
                else if ((currLink >> 8) > 0 && (currLink & IRRELEVANT) == 0)  
                    dest[i] = (UChar)(0xFE70 + (currLink >> 8) + Shape);
            }
        }

        /* move one notch forward */
        if ((currLink & IRRELEVANT) == 0) {
            prevLink = lastLink;
            lastLink = currLink;
            lastPos = i;
        }

        i = i + step;
        if (i == Nx) {
            currLink = nextLink;
            Nx = -2;
        } else if(i != iend) {
            currLink = getLink(dest[i]);
        }
    }

    if(lamalef_found != 0)
        destSize = removeLamAlefSpaces(dest,sourceLength,destSize,options,pErrorCode);
    else
        destSize = sourceLength;

    return destSize;
}

/*
 *Name     : deShapeUnicode
 *Function : Converts an Arabic Unicode buffer in FExx Range into unshaped
 *           arabic Unicode buffer in 06xx Range
 */
static int32_t
deShapeUnicode(UChar *dest, int32_t sourceLength,
               int32_t destSize,uint32_t options,
               UErrorCode *pErrorCode) {
    int32_t i = 0;
    int32_t lamalef_found = 0;

    /*
     *This for loop changes the buffer from the Unicode FE range to
     *the Unicode 06 range
     */
    for(i = 0; i < sourceLength; i++) {
        UChar  inputChar = dest[i];
        if ( (inputChar >= 0xFB50) && (inputChar <= 0xFBFF)) { /* FBxx Arabic range */
            UChar c = convertFBto06 [ (inputChar - 0xFB50) ];
            if (c != 0)
                dest[i] = c;
        } else if (( inputChar >= 0xFE70) && (inputChar <= 0xFEF4 )) { /* FExx Arabic range */
            dest[i] = convertFEto06 [ (inputChar - 0xFE70) ]  ;
        } else {
            dest[i] = inputChar ;
        }
        if( isLamAlefChar(dest[i]) )
            lamalef_found = 1;
    }

    /* If there is lamalef in the buffer call expandLamAlef */
    if(lamalef_found != 0)
        destSize = expandLamAlef(dest,sourceLength,destSize,options,pErrorCode);
    else
        destSize = sourceLength;

    return destSize;
}

U_CAPI int32_t U_EXPORT2
u_shapeArabic(const UChar *source, int32_t sourceLength,
              UChar *dest, int32_t destCapacity,
              uint32_t options,
              UErrorCode *pErrorCode) {

    int32_t destLength;

    /* usual error checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* make sure that no reserved options values are used; allow dest==NULL only for preflighting */
    if( source==NULL || sourceLength<-1 || (dest==NULL && destCapacity!=0) || destCapacity<0 ||
                (options&U_SHAPE_DIGIT_TYPE_RESERVED)==U_SHAPE_DIGIT_TYPE_RESERVED ||
                (options&U_SHAPE_DIGITS_MASK)==U_SHAPE_DIGITS_RESERVED ||
                ((options&U_SHAPE_LENGTH_MASK) != U_SHAPE_LENGTH_GROW_SHRINK  &&
                (options&U_SHAPE_AGGREGATE_TASHKEEL_MASK) != 0) ||
                ((options&U_SHAPE_AGGREGATE_TASHKEEL_MASK) == U_SHAPE_AGGREGATE_TASHKEEL &&
                (options&U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED) != U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED)
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
        
    /* determine the source length */
    if(sourceLength==-1) {
        sourceLength=u_strlen(source);
    }
    if(sourceLength<=0) {
        return u_terminateUChars(dest, destCapacity, 0, pErrorCode);
    }

    /* check that source and destination do not overlap */
    if( dest!=NULL &&
        ((source<=dest && dest<source+sourceLength) ||
         (dest<=source && source<dest+destCapacity))) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if((options&U_SHAPE_LETTERS_MASK)!=U_SHAPE_LETTERS_NOOP) {
        UChar buffer[300];
        UChar *tempbuffer, *tempsource = NULL;
        int32_t outputSize, spacesCountl=0, spacesCountr=0;

        if((options&U_SHAPE_AGGREGATE_TASHKEEL_MASK)>0) {
            int32_t logical_order = (options&U_SHAPE_TEXT_DIRECTION_MASK) == U_SHAPE_TEXT_DIRECTION_LOGICAL;
            int32_t aggregate_tashkeel = 
                        (options&(U_SHAPE_AGGREGATE_TASHKEEL_MASK+U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED)) == 
                        (U_SHAPE_AGGREGATE_TASHKEEL+U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED);
            int step=logical_order?1:-1;
            int j=logical_order?-1:2*sourceLength;
            int i=logical_order?-1:sourceLength;
            int end=logical_order?sourceLength:-1;
            int aggregation_possible = 1;
            UChar prev = 0;
            UChar prevLink, currLink = 0;
            int newSourceLength = 0;
            tempsource = (UChar *)uprv_malloc(2*sourceLength*U_SIZEOF_UCHAR);
            if(tempsource == NULL) {
                *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }

            while ((i+=step) != end) {
                prevLink = currLink;
                currLink = getLink(source[i]);
                if (aggregate_tashkeel && ((prevLink|currLink)&COMBINE) == COMBINE && aggregation_possible) {
                    aggregation_possible = 0; 
                    tempsource[j] = (prev<source[i]?prev:source[i])-0x064C+0xFC5E;
                    currLink = getLink(tempsource[j]);
                } else {
                    aggregation_possible = 1;
                    tempsource[j+=step] = source[i];
                    prev = source[i];
                    newSourceLength++;
                }
            }
            source = tempsource+(logical_order?0:j);
            sourceLength = newSourceLength;
        }

        /* calculate destination size */
        /* TODO: do we ever need to do this pure preflighting? */
        if((options&U_SHAPE_LENGTH_MASK)==U_SHAPE_LENGTH_GROW_SHRINK) {
            outputSize=calculateSize(source,sourceLength,destCapacity,options);
        } else {
            outputSize=sourceLength;
        }
        if(outputSize>destCapacity) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                if (tempsource != NULL) uprv_free(tempsource);
            return outputSize;
        }

        /*
         * need a temporary buffer of size max(outputSize, sourceLength)
         * because at first we copy source->temp
         */
        if(sourceLength>outputSize) {
            outputSize=sourceLength;
        }

        /* Start of Arabic letter shaping part */
        if(outputSize<=sizeof(buffer)/U_SIZEOF_UCHAR) {
            outputSize=sizeof(buffer)/U_SIZEOF_UCHAR;
            tempbuffer=buffer;
        } else {
            tempbuffer = (UChar *)uprv_malloc(outputSize*U_SIZEOF_UCHAR);

            /*Test for NULL*/
            if(tempbuffer == NULL) {
                *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
                if (tempsource != NULL) uprv_free(tempsource);
                return 0;
            }
        }
        uprv_memcpy(tempbuffer, source, sourceLength*U_SIZEOF_UCHAR);
        if (tempsource != NULL) uprv_free(tempsource);
        if(sourceLength<outputSize) {
            uprv_memset(tempbuffer+sourceLength, 0, (outputSize-sourceLength)*U_SIZEOF_UCHAR);
        }

        if((options&U_SHAPE_TEXT_DIRECTION_MASK) == U_SHAPE_TEXT_DIRECTION_LOGICAL) {
            countSpaces(tempbuffer,sourceLength,options,&spacesCountl,&spacesCountr);
            invertBuffer(tempbuffer,sourceLength,options,spacesCountl,spacesCountr);
        }

        switch(options&U_SHAPE_LETTERS_MASK) {
        case U_SHAPE_LETTERS_SHAPE :
            /* Call the shaping function with tashkeel flag == 1 */
            destLength = shapeUnicode(tempbuffer,sourceLength,destCapacity,options,pErrorCode,1);
            break;
        case U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED :
            /* Call the shaping function with tashkeel flag == 0 */
            destLength = shapeUnicode(tempbuffer,sourceLength,destCapacity,options,pErrorCode,0);
            break;
        case U_SHAPE_LETTERS_UNSHAPE :
            /* Call the deshaping function */
            destLength = deShapeUnicode(tempbuffer,sourceLength,destCapacity,options,pErrorCode);
            break;
        default :
            /* will never occur because of validity checks above */
            destLength = 0;
            break;
        }

        /*
         * TODO: (markus 2002aug01)
         * For as long as we always preflight the outputSize above
         * we should U_ASSERT(outputSize==destLength)
         * except for the adjustment above before the tempbuffer allocation
         */

        if((options&U_SHAPE_TEXT_DIRECTION_MASK) == U_SHAPE_TEXT_DIRECTION_LOGICAL) {
            countSpaces(tempbuffer,destLength,options,&spacesCountl,&spacesCountr);
            invertBuffer(tempbuffer,destLength,options,spacesCountl,spacesCountr);
        }
        uprv_memcpy(dest, tempbuffer, uprv_min(destLength, destCapacity)*U_SIZEOF_UCHAR);

        if(tempbuffer!=buffer) {
            uprv_free(tempbuffer);
        }

        if(destLength>destCapacity) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return destLength;
        }

        /* End of Arabic letter shaping part */
    } else {
        /*
         * No letter shaping:
         * just make sure the destination is large enough and copy the string.
         */
        if(destCapacity<sourceLength) {
            /* this catches preflighting, too */
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return sourceLength;
        }
        uprv_memcpy(dest, source, sourceLength*U_SIZEOF_UCHAR);
        destLength=sourceLength;
    }

    /*
     * Perform number shaping.
     * With UTF-16 or UTF-32, the length of the string is constant.
     * The easiest way to do this is to operate on the destination and
     * "shape" the digits in-place.
     */
    if((options&U_SHAPE_DIGITS_MASK)!=U_SHAPE_DIGITS_NOOP) {
        UChar digitBase;
        int32_t i;

        /* select the requested digit group */
        switch(options&U_SHAPE_DIGIT_TYPE_MASK) {
        case U_SHAPE_DIGIT_TYPE_AN:
            digitBase=0x660; /* Unicode: "Arabic-Indic digits" */
            break;
        case U_SHAPE_DIGIT_TYPE_AN_EXTENDED:
            digitBase=0x6f0; /* Unicode: "Eastern Arabic-Indic digits (Persian and Urdu)" */
            break;
        default:
            /* will never occur because of validity checks above */
            digitBase=0;
            break;
        }

        /* perform the requested operation */
        switch(options&U_SHAPE_DIGITS_MASK) {
        case U_SHAPE_DIGITS_EN2AN:
            /* add (digitBase-'0') to each European (ASCII) digit code point */
            digitBase-=0x30;
            for(i=0; i<destLength; ++i) {
                if(((uint32_t)dest[i]-0x30)<10) {
                    dest[i]+=digitBase;
                }
            }
            break;
        case U_SHAPE_DIGITS_AN2EN:
            /* subtract (digitBase-'0') from each Arabic digit code point */
            for(i=0; i<destLength; ++i) {
                if(((uint32_t)dest[i]-(uint32_t)digitBase)<10) {
                    dest[i]-=digitBase-0x30;
                }
            }
            break;
        case U_SHAPE_DIGITS_ALEN2AN_INIT_LR:
            _shapeToArabicDigitsWithContext(dest, destLength,
                                            digitBase,
                                            (UBool)((options&U_SHAPE_TEXT_DIRECTION_MASK)==U_SHAPE_TEXT_DIRECTION_LOGICAL),
                                            FALSE);
            break;
        case U_SHAPE_DIGITS_ALEN2AN_INIT_AL:
            _shapeToArabicDigitsWithContext(dest, destLength,
                                            digitBase,
                                            (UBool)((options&U_SHAPE_TEXT_DIRECTION_MASK)==U_SHAPE_TEXT_DIRECTION_LOGICAL),
                                            TRUE);
            break;
        default:
            /* will never occur because of validity checks above */
            break;
        }
    }

    return u_terminateUChars(dest, destCapacity, destLength, pErrorCode);
}
