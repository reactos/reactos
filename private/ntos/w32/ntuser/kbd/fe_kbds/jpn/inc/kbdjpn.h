/****************************** Module Header ******************************\
* Module Name: kbdjpn.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
\***************************************************************************/
/*
 * Katakana Unicode
 */
enum _KATAKANA_UNICODE {
    WCH_IP=0xff61, // Ideographic Period
    WCH_OB,        // Opening Corner Bracket
    WCH_CB,        // Closing Corner Bracket
    WCH_IC,        // Ideographic Comma
    WCH_MD,        // Katakana Middle Dot
    WCH_WO,        // Katakana Letter WO
    WCH_AA,        // Katakana Letter Small A
    WCH_II,        // Katakana Letter Small I
    WCH_UU,        // Katakana Letter Small U
    WCH_EE,        // Katakana Letter Small E
    WCH_OO,        // Katakana Letter Small O
    WCH_YAA,       // Katakana Letter Small YA
    WCH_YUU,       // Katakana Letter Small YU
    WCH_YOO,       // Katakana Letter Small YO
    WCH_TUU,       // Katakana Letter Small TU
    WCH_PS,        // Katakana Prolonged Sound Mark
    WCH_A,         // Katakana Letter A
    WCH_I,         // Katakana Letter I
    WCH_U,         // Katakana Letter U
    WCH_E,         // Katakana Letter E
    WCH_O,         // Katakana Letter O
    WCH_KA,        // Katakana Letter KA
    WCH_KI,        // Katakana Letter KI
    WCH_KU,        // Katakana Letter KU
    WCH_KE,        // Katakana Letter KE
    WCH_KO,        // Katakana Letter KO
    WCH_SA,        // Katakana Letter SA
    WCH_SI,        // Katakana Letter SI
    WCH_SU,        // Katakana Letter SU
    WCH_SE,        // Katakana Letter SE
    WCH_SO,        // Katakana Letter SO
    WCH_TA,        // Katakana Letter TA
    WCH_TI,        // Katakana Letter TI
    WCH_TU,        // Katakana Letter TU
    WCH_TE,        // Katakana Letter TE
    WCH_TO,        // Katakana Letter TO
    WCH_NA,        // Katakana Letter NA
    WCH_NI,        // Kanakana Letter NI
    WCH_NU,        // Katakana Letter NU
    WCH_NE,        // Katakana Letter NE
    WCH_NO,        // Katakana Letter NO
    WCH_HA,        // Katakana Letter HA
    WCH_HI,        // Katakana Letter HI
    WCH_HU,        // Katakana Letter HU
    WCH_HE,        // Katakana Letter HE
    WCH_HO,        // Katakana Letter HO
    WCH_MA,        // Katakana Letter MA
    WCH_MI,        // Katakana Letter MI
    WCH_MU,        // Katakana Letter MU
    WCH_ME,        // Katakana Letter ME
    WCH_MO,        // Katakana Letter MO
    WCH_YA,        // Katakana Letter YA
    WCH_YU,        // Katakana Letter YU
    WCH_YO,        // Katakana Letter YO
    WCH_RA,        // Katakana Letter RA
    WCH_RI,        // Katakana Letter RI
    WCH_RU,        // Katakana Letter RU
    WCH_RE,        // Katakana Letter RE
    WCH_RO,        // Katakana Letter RO
    WCH_WA,        // Katakana Letter WA
    WCH_NN,        // Katakana Letter N
    WCH_VS,        // Katakana Voiced Sound Mark
    WCH_SVS        // Katakana Semi-Voiced Sound Mark
};

/***************************************************************************\
* OEM Key Name -
\***************************************************************************/

                                    // lo  hi  lo  hi
#define SZ_KEY_NAME_HENKAN          "\x09\x59\xdb\x63\000\000"
#define SZ_KEY_NAME_MUHENKAN        "\x21\x71\x09\x59\xdb\x63\000\000"
#define SZ_KEY_NAME_KANJI           "\x22\x6f\x57\x5b\000\000"
#define SZ_KEY_NAME_EISU_KANA       "\xf1\x82\x70\x65\x20\000\xab\x30\xca\x30\000\000"
#define SZ_KEY_NAME_HANKAKU_ZENKAKU "\x4a\x53\xd2\x89\x2f\000\x68\x51\xd2\x89\000\000"
#define SZ_KEY_NAME_KATAKANA        "\xab\x30\xbf\x30\xab\x30\xca\x30\000\000"
#define SZ_KEY_NAME_HIRAGANA        "\x72\x30\x89\x30\x4c\x30\x6a\x30\000\000"
// FMR Jul.13.1994 KA
// For the GetKeyNameText() API function.
#define SZ_KEY_NAME_BACKSPACE       "\x8C\x5F\x00\x90\000\000"
#define SZ_KEY_NAME_ENTER           "\x39\x65\x4C\x88\000\000"
#define SZ_KEY_NAME_NUMPADENTER     "\x4E\x00\x75\x00\x6d\x00\x20\x00\x39\x65\x4C\x88\000\000"
#define SZ_KEY_NAME_SPACE           "\x7A\x7A\x7D\x76\000\000"
#define SZ_KEY_NAME_INSERT          "\x3F\x63\x65\x51\000\000"
#define SZ_KEY_NAME_DELETE          "\x4A\x52\x64\x96\000\000"
#define SZ_KEY_NAME_KANAKANJI       "\x4b\x30\x6a\x30\x22\x6f\x57\x5b\000\000"
#define SZ_KEY_NAME_SHIFTLEFT       "\xB7\x30\xD5\x30\xC8\x30\xE6\x5D\000\000"
#define SZ_KEY_NAME_SHIFTRIGHT      "\xB7\x30\xD5\x30\xC8\x30\xF3\x53\000\000"
#define SZ_KEY_NAME_EIJI            "\xF1\x82\x57\x5B\000\000"
#define SZ_KEY_NAME_JISHO           "\x58\x53\x9E\x8A\x9E\x8F\xF8\x66\000\000"
#define SZ_KEY_NAME_MASSHOU         "\x58\x53\x9E\x8A\xB9\x62\x88\x6D\000\000"
#define SZ_KEY_NAME_TOUROKU         "\x58\x53\x9E\x8A\x7B\x76\x32\x93\000\000"
#define SZ_KEY_NAME_PRIOR           "\x4D\x52\x4C\x88\000\000"
#define SZ_KEY_NAME_NEXT            "\x21\x6B\x4C\x88\000\000"
#define SZ_KEY_NAME_CANCEL          "\xD6\x53\x88\x6D\000\000"
#define SZ_KEY_NAME_EXECUTE         "\x9F\x5B\x4C\x88\000\000"
#define SZ_KEY_NAME_TAB             "\xBF\x30\xD6\x30\000\000"


//----------------------[ NEC Code Original Start ]-----------------
                    // ff76(ka) ff85(na) for Unicode
#define SZ_KEY_NAME_KANA        "\x76\xff\x85\xff"
#define SZ_KEY_NAME_F1          "\x66\x00\x65\xff\x31\x00"
#define SZ_KEY_NAME_F2          "\x66\x00\x65\xff\x32\x00"
#define SZ_KEY_NAME_F3          "\x66\x00\x65\xff\x33\x00"
#define SZ_KEY_NAME_F4          "\x66\x00\x65\xff\x34\x00"
#define SZ_KEY_NAME_F5          "\x66\x00\x65\xff\x35\x00"
#define SZ_KEY_NAME_F6          "\x66\x00\x65\xff\x36\x00"
#define SZ_KEY_NAME_F7          "\x66\x00\x65\xff\x37\x00"
#define SZ_KEY_NAME_F8          "\x66\x00\x65\xff\x38\x00"
#define SZ_KEY_NAME_F9          "\x66\x00\x65\xff\x39\x00"
#define SZ_KEY_NAME_F10         "\x66\x00\x65\xff\x31\x00\x30\x00"
#define SZ_KEY_NAME_F11         "\x66\x00\x65\xff\x31\x00\x31\x00"
#define SZ_KEY_NAME_F12         "\x66\x00\x65\xff\x31\x00\x32\x00"
#define SZ_KEY_NAME_F13         "\x66\x00\x65\xff\x31\x00\x33\x00"
#define SZ_KEY_NAME_F14         "\x66\x00\x65\xff\x31\x00\x34\x00"
#define SZ_KEY_NAME_F15         "\x66\x00\x65\xff\x31\x00\x35\x00"

//----------------------[ NEC Code Original Start ]-----------------
//This is NEC Document Processer define
//
#define SZ_KEY_NAME_DP_ZENKAKU_HANKAKU "\x68\x51\xd2\x89\x2f\000\x4a\x53\xd2\x89\000\000"
#define SZ_KEY_NAME_DP_KANA            "\x4b\x30\x6a\x30\000\000"
#define SZ_KEY_NAME_DP_KATAKANA        "\xab\x30\xbf\x30\xab\x30\xca\x30\000\000"
#define SZ_KEY_NAME_DP_EISU            "\xf1\x82\x70\x65\000\000"
