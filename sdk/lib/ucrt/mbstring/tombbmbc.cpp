/***
*tombbmbc.c - convert 1-byte code to and from 2-byte code
*
*       Copyright (c) Microsoft Corporation.    All rights reserved.
*
*Purpose:
*       _mbbtombc() - converts 1-byte code to corresponding 2-byte code
*       _mbctombb() - converts 2-byte code to corresponding 1-byte code
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>


#define ASCLOW   0x20
#define ASCHIGH  0x7e

#define SBLOW   0xA1
#define SBHIGH  0xDF

#define MBLIMIT 0x8396

static unsigned short const mbbtable[] = {
        /*20*/  0x8140, 0x8149, 0x8168, 0x8194, 0x8190, 0x8193, 0x8195, 0x8166,
                0x8169, 0x816a, 0x8196, 0x817b, 0x8143, 0x817c, 0x8144, 0x815e,
        /*30*/  0x824f, 0x8250, 0x8251, 0x8252, 0x8253, 0x8254, 0x8255, 0x8256,
                0x8257, 0x8258, 0x8146, 0x8147, 0x8183, 0x8181, 0x8184, 0x8148,
        /*40*/  0x8197, 0x8260, 0x8261, 0x8262, 0x8263, 0x8264, 0x8265, 0x8266,
                0x8267, 0x8268, 0x8269, 0x826a, 0x826b, 0x826c, 0x826d, 0x826e,
        /*50*/  0x826f, 0x8270, 0x8271, 0x8272, 0x8273, 0x8274, 0x8275, 0x8276,
                0x8277, 0x8278, 0x8279, 0x816d, 0x818f, 0x816e, 0x814f, 0x8151,
        /*60*/  0x8165, 0x8281, 0x8282, 0x8283, 0x8284, 0x8285, 0x8286, 0x8287,
                0x8288, 0x8289, 0x828a, 0x828b, 0x828c, 0x828d, 0x828e, 0x828f,
        /*70*/  0x8290, 0x8291, 0x8292, 0x8293, 0x8294, 0x8295, 0x8296, 0x8297,
                0x8298, 0x8299, 0x829a, 0x816f, 0x8162, 0x8170, 0x8150,
};

static struct   {
    unsigned char   asc;
    char            synonym;
    unsigned short  mbccode;
} const mbctable[] = {
    //     ASCII Code | Synonym | KANJI Code
//Katakana Table
        {    0xA7,         1,       0x8340      },  //  'a'
        {    0xB1,         1,       0x8341      },  //  'A'
        {    0xA8,         1,       0x8342      },  //  'i'
        {    0xB2,         1,       0x8343      },  //  'I'
        {    0xA9,         1,       0x8344      },  //  'u'
        {    0xB3,         1,       0x8345      },  //  'U'
        {    0xAA,         1,       0x8346      },  //  'e'
        {    0xB4,         1,       0x8347      },  //  'E'
        {    0xAB,         1,       0x8348      },  //  'o'
        {    0xB5,         1,       0x8349      },  //  'O'

        {    0xB6,         2,       0x834A      },  //  'KA'
        {    0xB7,         2,       0x834C      },  //  'KI'
        {    0xB8,         2,       0x834E      },  //  'KU'
        {    0xB9,         2,       0x8350      },  //  'KE'
        {    0xBA,         2,       0x8352      },  //  'KO'

        {    0xBB,         2,       0x8354      },  //  'SA'
        {    0xBC,         2,       0x8356      },  //  'SI'
        {    0xBD,         2,       0x8358      },  //  'SU'
        {    0xBE,         2,       0x835A      },  //  'SE'
        {    0xBF,         2,       0x835C      },  //  'SO'

        {    0xC0,         2,       0x835E      },  //  'TA'
        {    0xC1,         2,       0x8360      },  //  'CHI'
        {    0xAF,         1,       0x8362      },  //  'tsu'
        {    0xC2,         2,       0x8363      },  //  'TSU'
        {    0xC3,         2,       0x8365      },  //  'TE''
        {    0xC4,         2,       0x8367      },  //  'TO''

        {    0xC5,         1,       0x8369      },  //  'NA'
        {    0xC6,         1,       0x836A      },  //  'NI'
        {    0xC7,         1,       0x836B      },  //  'NU'
        {    0xC8,         1,       0x836C      },  //  'NE'
        {    0xC9,         1,       0x836D      },  //  'NO'

        {    0xCA,         3,       0x836E      },  //  'HA'
        {    0xCB,         3,       0x8371      },  //  'HI'
        {    0xCC,         3,       0x8374      },  //  'FU'
        {    0xCD,         3,       0x8377      },  //  'HE'
        {    0xCE,         3,       0x837A      },  //  'HO'

        {    0xCF,         1,       0x837D      },  //  'MA'
        {    0xD0,         1,       0x837E      },  //  'MI'
        {    0xD1,         1,       0x8380      },  //  'MU'
        {    0xD2,         1,       0x8381      },  //  'ME'
        {    0xD3,         1,       0x8382      },  //  'MO'

        {    0xAC,         1,       0x8383      },  //  'ya'
        {    0xD4,         1,       0x8384      },  //  'YA'
        {    0xAD,         1,       0x8385      },  //  'yu'
        {    0xD5,         1,       0x8386      },  //  'YU'
        {    0xAE,         1,       0x8387      },  //  'yo'
        {    0xD6,         1,       0x8388      },  //  'YO'

        {    0xD7,         1,       0x8389      },  //  'RA'
        {    0xD8,         1,       0x838A      },  //  'RI'
        {    0xD9,         1,       0x838B      },  //  'RU'
        {    0xDA,         1,       0x838C      },  //  'RE'
        {    0xDB,         1,       0x838D      },  //  'RO'

        {    0xDC,         2,       0x838E      },  //  'WA'
        {    0xB2,         1,       0x8390      },  //  'I'
        {    0xB4,         1,       0x8391      },  //  'E'

        {    0xA6,         1,       0x8392      },  //  'WO'
        {    0xDD,         1,       0x8393      },  //  'N'

        {    0xB3,         1,       0x8394      },  //  'U'
        {    0xB6,         1,       0x8395      },  //  'KA'
        {    0xB9,         1,       0x8396      },  //  'KE'

// Hiragana Table
        {    0xA7,         1,       0x829F      },  //  'a'
        {    0xB1,         1,       0x82A0      },  //  'A'
        {    0xA8,         1,       0x82A1      },  //  'i'
        {    0xB2,         1,       0x82A2      },  //  'I'
        {    0xA9,         1,       0x82A3      },  //  'u'
        {    0xB3,         1,       0x82A4      },  //  'U'
        {    0xAA,         1,       0x82A5      },  //  'e'
        {    0xB4,         1,       0x82A6      },  //  'E'
        {    0xAB,         1,       0x82A7      },  //  'o'
        {    0xB5,         1,       0x82A8      },  //  'O'

        {    0xB6,         2,       0x82A9      },  //  'KA'
        {    0xB7,         2,       0x82AB      },  //  'KI'
        {    0xB8,         2,       0x82AD      },  //  'KU'
        {    0xB9,         2,       0x82AF      },  //  'KE'
        {    0xBA,         2,       0x82B1      },  //  'KO'

        {    0xBB,         2,       0x82B3      },  //  'SA'
        {    0xBC,         2,       0x82B5      },  //  'SI'
        {    0xBD,         2,       0x82B7      },  //  'SU'
        {    0xBE,         2,       0x82B9      },  //  'SE'
        {    0xBF,         2,       0x82BB      },  //  'SO'

        {    0xC0,         2,       0x82BD      },  //  'TA'
        {    0xC1,         2,       0x82BF      },  //  'CHI'
        {    0xAF,         1,       0x82C1      },  //  'tsu'
        {    0xC2,         2,       0x82C2      },  //  'TSU'
        {    0xC3,         2,       0x82C4      },  //  'TE'
        {    0xC4,         2,       0x82C6      },  //  'TO'

        {    0xC5,         1,       0x82C8      },  //  'NA'
        {    0xC6,         1,       0x82C9      },  //  'NI'
        {    0xC7,         1,       0x82CA      },  //  'NU'
        {    0xC8,         1,       0x82CB      },  //  'NE'
        {    0xC9,         1,       0x82CC      },  //  'NO'

        {    0xCA,         3,       0x82CD      },  //  'HA'
        {    0xCB,         3,       0x82D0      },  //  'HI'
        {    0xCC,         3,       0x82D3      },  //  'FU'
        {    0xCD,         3,       0x82D6      },  //  'HE'
        {    0xCE,         3,       0x82D9      },  //  'HO'

        {    0xCF,         1,       0x82DC      },  //  'MA'
        {    0xD0,         1,       0x82DD      },  //  'MI'
        {    0xD1,         1,       0x82DE      },  //  'MU'
        {    0xD2,         1,       0x82DF      },  //  'ME'
        {    0xD3,         1,       0x82E0      },  //  'MO'

        {    0xAC,         1,       0x82E1      },  //  'ya'
        {    0xD4,         1,       0x82E2      },  //  'YA'
        {    0xAD,         1,       0x82E3      },  //  'yu'
        {    0xD5,         1,       0x82E4      },  //  'YU'
        {    0xAE,         1,       0x82E5      },  //  'yo'
        {    0xD6,         1,       0x82E6      },  //  'YO'

        {    0xD7,         1,       0x82E7      },  //  'RA'
        {    0xD8,         1,       0x82E8      },  //  'RI'
        {    0xD9,         1,       0x82E9      },  //  'RU'
        {    0xDA,         1,       0x82EA      },  //  'RE'
        {    0xDB,         1,       0x82EB      },  //  'RO'

        {    0xDC,         2,       0x82EC      },  //  'WA'
        {    0xB2,         1,       0x82EE      },  //  'I'
        {    0xB4,         1,       0x82EF      },  //  'E'

        {    0xA6,         1,       0x82F0      },  //  'WO'
        {    0xDD,         1,       0x82F1      },  //  'N'

        {    0x20,         1,       0x8140      },  // ' '
//      {    0xA0,         1,       0x8140      },  // ' '
        {    0xA1,         1,       0x8142      },  //
        {    0xA2,         1,       0x8175      },  //
        {    0xA3,         1,       0x8176      },  //
        {    0xA4,         1,       0x8141      },  //
        {    0xA5,         1,       0x8145      },  //
        {    0xB0,         1,       0x815b      },  //  '-'
        {    0xDE,         1,       0x814a      },  //
        {    0xDF,         1,       0x814b      },  //

        {    0,            0,       0           }   // == End of Table

};

/***
*unsigned int _mbbtombc(c) - convert mbbvalue to mbcvalue.
*
*Purpose:
*       Converts mbbvalue (1-byte) to corresponding mbcvalue code (2-byte).
*
*Entry:
*       unsigned int c - mbbvalue character code to be converted.
*
*Exit:
*       Returns corresponding mbbvalue (2-byte).
*
*Exceptions:
*       Returns c if corresponding 2-byte code does not exist.
*
*******************************************************************************/

extern "C" unsigned int __cdecl _mbbtombc_l(
        unsigned int c,
        _locale_t plocinfo
        )
{
    int i;
    _LocaleUpdate _loc_update(plocinfo);

    if (_loc_update.GetLocaleT()->mbcinfo->mbcodepage != _KANJI_CP)
        return (c);

    /* If c is in the ASCII range, then look up the corresponding value
     * in the mbbtable. */

    if (c >= ASCLOW && c <= ASCHIGH)
        return (mbbtable[c-ASCLOW]);

    /* Exception for KANJI */

    if (c == 0xdc)
        return( 0x838f );

    /* If c is a Katakana character, lookup in mbctable. */

    if (c >= SBLOW && c <= SBHIGH)
    {
        for(i = 0; mbctable[i].asc != 0; i++)
        {
            if ( c == (unsigned int)mbctable[i].asc ) {
                c = (unsigned int)mbctable[i].mbccode ;
                break;
                    }
        }
    }

        return(c);
}

extern "C" unsigned int (__cdecl _mbbtombc)(
    unsigned int c
    )
{
    return _mbbtombc_l(c, nullptr);
}

/***
*unsigned int _mbctombb(c) - convert mbcvalue to mbbvalue.
*
*Purpose:
*       Converts mbcvalue (2-byte) to corresponding mbbvalue (1-byte).
*
*Entry:
*       unsigned int c - mbcvalue character code to convert.
*
*Exit:
*       Returns corresponding mbbvalue (1-byte).
*
*Exceptions:
*       Returns c if corresponding 1-byte code does not exist.
*
*******************************************************************************/

extern "C" unsigned int __cdecl _mbctombb_l(
        unsigned int c,
        _locale_t plocinfo
        )
{
   int i;
   int result;
    _LocaleUpdate _loc_update(plocinfo);

    if (_loc_update.GetLocaleT()->mbcinfo->mbcodepage != _KANJI_CP)
        return (c);

   /* Check to see if c is in the ASCII range.  */

    for (i = 0; i <= ASCHIGH - ASCLOW; i++)
    {
        if (c == (unsigned int)mbbtable[i])
            return((unsigned int)i + ASCLOW);
    }


   /*  If c is a valid MBCS value, search the mbctable for value. */

    if ( c <= MBLIMIT )
    {
        for (i = 0; mbctable[i].asc ; i++)
        {
            result = (int)c - (int)mbctable[i].mbccode;
            if (result == 0)
                return( (unsigned int)mbctable[i].asc );
            else if (((c & 0xff00) == (unsigned int)(mbctable[i].mbccode & 0xff00))
            && (result > 0)
            && ((result - mbctable[i].synonym) < 0))
                return( (unsigned int)mbctable[i].asc );
        }
    }

    return(c);
}

extern "C" unsigned int (__cdecl _mbctombb)(
    unsigned int c
    )
{
    return _mbctombb_l(c, nullptr);
}
