/*
 * @(#)CharType.cxx 1.0 6/15/98
 * 
 * Character type constants and functions
 *
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

int g_anCharType[TABLE_SIZE] = { 
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0 | FWHITESPACE | FCHARDATA,
    0 | FWHITESPACE | FCHARDATA,
    0,
    0,
    0 | FWHITESPACE | FCHARDATA,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0 | FWHITESPACE | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FMISCNAME | FCHARDATA,
    0 | FMISCNAME | FCHARDATA,
    0 | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FDIGIT | FCHARDATA,
    0 | FSTARTNAME | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FMISCNAME | FSTARTNAME | FCHARDATA,
    0 | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FLETTER | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
    0 | FCHARDATA,
};

const WCHAR * ParseName(const WCHAR *pwc, ULONG * pul)
{
    WCHAR ch;
    Assert(pwc);
    // we don't allow a colon at the beginning

    if ((ch = *pwc) && isStartNameChar(ch))
    {
        pwc = ParseNameToken(pwc, pul);
    }
    else
    {
        *pul = 0;
    }

    return pwc;
}

const WCHAR * ParseNameToken(const WCHAR *pwc, ULONG * pul)
{
    WCHAR ch;
    Assert(pwc);
    const WCHAR * pwcStart = pwc;
    const WCHAR * pwcColon = null;

    *pul = 0;

    if (*pwc == ':')
    {
        return pwc;
    }

    while((ch = *pwc) != 0 && isNameChar(ch))
    {
        if (ch == ':')
        {
            if (pwcColon)
                break;

            pwcColon = pwc;
        }

        pwc++;
    }

    // don't allow 0 length names

    if (pwcColon)
    {
        if (pwc - pwcColon == 1)
        {
            pwc = pwcColon;
        }
        else
        {
            *pul = (ULONG)(pwcColon - pwcStart);
        }
    }

    return pwc;
}


bool isValidName(const WCHAR *pwc, ULONG * pnColonPos)
{
    const WCHAR * pwchNext = ParseName(pwc, pnColonPos);

    if (*pwchNext || pwchNext == pwc)
    {
        return false;
    }

    return true;
}


//
// LanguageID ::= (IANACode || UserCode || ISO639) ('-' SubCode)*
// ISO639 ::= ([a-z] | [A-Z])([a-z] | [A-Z])
// IANACode ::= ('i' | 'I') '-' ([a-z] | [A-Z])+
// UserCode ::= ('x' | 'X') '-' ([a-z] | [A-Z])+
// SubCode :;= ([a-z] | [A-Z])+
//
bool
isValidLanguageID(const WCHAR * pwcText, ULONG ulLen)
{
    WCHAR ch;
    bool fSeenLetter = false;

    if (ulLen < 2)
        goto Error;

    ch = *pwcText;
    if (isLetter(ch))
    {
        if (isLetter(*(++pwcText))) // ISO639 Code: a two-letter language code as defined by ISO 639
        {
            if (2 == ulLen)
                return true;
            ulLen--;
            pwcText++;
        }
        else if ('I' != ch && 'i' != ch && //IANA Code
                 'X' != ch && 'x' != ch)    // User Code
        {
            goto Error;
        }

        if ('-' != *pwcText)
            goto Error;
        ulLen -= 2;

        while (ulLen-- > 0)
        {
            ch = *(++pwcText);
            if (isLetter(ch))
            {
                fSeenLetter = true;
            }
            else if ('-' == ch && fSeenLetter)
            {
                fSeenLetter = false;
            }
            else
            {
                goto Error;
            }
        }

        if (fSeenLetter)
            return true;
    }

Error:
    return false;
}


bool 
isValidPublicID(const WCHAR * pwcText, ULONG ulLen)
{
    // length of 0 is OK
    while (ulLen-- > 0)
    {
        if (!isPubidChar(*pwcText++))
            return false;
    }
    return true;
}

//==============================================================================
WCHAR BuiltinEntity(const WCHAR* text, ULONG len)
{
    ULONG ulength =  len * sizeof(WCHAR); // Length in chars
    switch (len)
    {
    case 4:
        if (::memcmp(L"quot", text, ulength) == 0)
        {
            return 34;
        }
        else if (::memcmp(L"apos", text, ulength) == 0)
        {
            return 39;
        }
        break;
    case 3:
        if (::memcmp(L"amp", text, ulength) == 0)
        {
            return 38;
        }
        break;
    case 2:
        if (::memcmp(L"lt", text, ulength) == 0)
        {
            return 60;
        }
        else if (::memcmp(L"gt", text, ulength) == 0)
        {
            return 62;
        }
        break;
    }
    return 0;
}

// BUGBUG -- need a better ifdef than this.
#ifdef UNIX
const ULONG MAXWCHAR = 0xFFFFFFFF;
#else
const ULONG MAXWCHAR = 0xFFFF;
#endif

//==============================================================================
HRESULT HexToUnicode(const WCHAR* text, ULONG len, WCHAR& ch)
{
    ULONG result = 0;
    for (ULONG i = 0; i < len; i++)
    {
        ULONG digit = 0;
        if (text[i] >= L'a' && text[i] <= L'f')
        {
            digit = 10 + (text[i] - L'a');
        }
        else if (text[i] >= L'A' && text[i] <= L'F')
        {
            digit = 10 + (text[i] - L'A');
        }
        else if (text[i] >= L'0' && text[i] <= L'9')
        {
            digit = (text[i] - L'0');
        }
        else
            return XML_E_INVALID_HEXIDECIMAL;

        // Last unicode value (MAXWCHAR) is reserved as "invalid value"
        if (result >= (MAXWCHAR - digit)/16)       // result is about to overflow
            return XML_E_INVALID_UNICODE;  // the maximum 4 byte value.

        result = (result*16) + digit;
    }
    if (result == 0)    // zero is also invalid.
        return XML_E_INVALID_UNICODE;
    ch = (WCHAR)result;
    return S_OK;
}

//==============================================================================
HRESULT DecimalToUnicode(const WCHAR* text, ULONG len, WCHAR& ch)
{
    ULONG result = 0;
    for (ULONG i = 0; i < len; i++)
    {
        ULONG digit = 0;
        if (text[i] >= L'0' && text[i] <= L'9')
        {
            digit = (text[i] - L'0');
        }
        else
            return XML_E_INVALID_DECIMAL;

        // Last unicode value (MAXWCHAR) is reserved as "invalid value"
        if (result >= (MAXWCHAR - digit) /10)       // result is about to overflow
            return XML_E_INVALID_UNICODE;          // the maximum 4 byte value.

        result = (result*10) + digit;
    }
    if (result == 0)    // zero is also invalid.
        return XML_E_INVALID_UNICODE;

    ch = (WCHAR)result;
    return S_OK;
}


#ifdef UNIX
    const unsigned char s_ByteOrderMarkTrident[sizeof(WCHAR)] = { 0xFF, 0xFE, 0xFF, 0xFE };
    const unsigned char s_ByteOrderMark[sizeof(WCHAR)] = { 0xFE, 0xFF, 0xFE, 0xFF };
    const unsigned char s_ByteOrderMarkUCS2[2] = { 0xFF, 0xFE };
#else
    const unsigned char s_ByteOrderMark[2] = { 0xFF, 0xFE };
#endif
