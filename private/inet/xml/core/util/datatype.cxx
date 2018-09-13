/*
 * @(#)DataType.cxx 1.0 6/11/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "datatype.hxx"
#include "chartype.hxx"

#ifdef UNIX
// Needed for _alloca
#include <malloc.h>
#endif // UNIX


TCHAR * pwszDataTypeNames[] =
{
    _T(""),                     // DT_NONE
    _T("string"),               // DT_STRING, DT_AV_CDATA string, pcdata VT_BSTR BSTR 
    _T("id"),                   // DT_AV_ID 
    _T("idref"),                // DT_AV_IDREF
    _T("idrefs"),               // DT_AV_IDREFS
    _T("entity"),               // DT_AV_ENTITY
    _T("entities"),             // DT_AV_ENTITIES
    _T("nmtoken"),              // DT_AV_NMTOKEN
    _T("nmtokens"),             // DT_AV_NMTOKENS
    _T("notation"),             // DT_AV_NOTATION
    _T("enumeration"),          // DT_AV_ENUMERATION
    _T("name"),                 // DT_AV_NAMEDEF - this should never be written out!

    _T("bin.base64"),           // DT_BASE64 - base64 as defined in the MIME IETF spec
    _T("bin.hex"),              // DT_BIN_HEX - Hexadecimal digits representing octets VT_ARRAY safearray or stream 
    _T("boolean"),              // DT_BOOLEAN - boolean, "1" or "0" VT_BOOL int 
    _T("char"),                 // DT_CHAR - char, string VT_UI2 wchar 
    _T("date"),                 // DT_DATE_ISO8601 - date.iso8601, A date in ISO 8601 format. (no time) VT_DATE long 
    _T("dateTime"),             // DT_DATETIME_ISO8601 - dateTime.iso8601, A date in ISO 8601 format, with optional time and no optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
    _T("dateTime.tz"),          // DT_DATETIME_ISO8601TZ - dateTime.iso8601tz, A date in ISO 8601 format, with optional time and optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
    _T("fixed.14.4"),           // DT_FIXED_14_4 - fixed.14.4, Same as "number" but no more than 14 digits to the left of the decimal point, and no more than 4 to the right. VT_CY large_integer 
    _T("float"),                // DT_FLOAT - float, Same as for "number." VT_R8 double 
    _T("float.IEEE.754.32"),    // DT_FLOAT_IEEE_754_32 - float.IEEE.754.32, " VT_FLOAT float 
    _T("float.IEEE.754.64"),    // DT_FLOAT_IEEE_754_64 - float.IEEE.754.64, " VT_DOUBLE double 
    _T("i1"),                   // DT_I1 - i1, A number, with optional sign, no fractions, no exponent. VT_I1 char 
    _T("i2"),                   // DT_I2 -  i2, " VT_I2 short 
    _T("i4"),                   // DT_I4 -  i4, " VT_I4 long
    _T("int"),                  // DT_INT -  int, A number, with optional sign, no fractions, no exponent. VT_I4 long 
    _T("number"),               // DT_NUMBER - number, A number, with no limit on digits, may potentially have a leading sign, fractional digits, and optionally an exponent. Punctuation as in US English. VT_R8 double 
    _T("r4"),                   // DT_R4 - r4, Same as "number." VT_FLOAT float 
    _T("r8"),                   // DT_R8 - r8, " VT_DOUBLE double 
    _T("time"),                 // DT_TIME_ISO8601 - time.iso8601, A time in ISO 8601 format, with no date and no time zone. VT_DATE long 
    _T("time.tz"),              // DT_TIME_ISO8601TZ - time.iso8601.tz, A time in ISO 8601 format, with no date but optional time zone. VT_DATE. long 
    _T("ui1"),                  // DT_UI1 - ui1, A number, unsigned, no fractions, no exponent. VT_UI1 unsigned char 
    _T("ui2"),                  // DT_UI2 - ui2, " VT_UI2 unsigned short 
    _T("ui4"),                  // DT_UI4 - ui4, " VT_UI4 unsigned long
    _T("uri"),                  // DT_URI - uri, Universal Resource Identifier VT_BSTR BSTR 
    _T("uuid"),                 // DT_UUID - uuid, Hexadecimal digits representing octets, optional embedded hyphens which should be ignored. VT_BSTR GUID 
    _T("?user-defined?"),
};

/**
 * Lookup table, sorted and all lowercase
 */
static const struct DTNAME
{
    TCHAR *     pszName;
    DataType    dt;
} s_adtinfo[] = 
{
    {_T("bin.base64"), DT_BASE64},  // base64 as defined in the MIME IETF spec
    {_T("bin.hex"), DT_BIN_HEX},    // Hexadecimal digits representing octets VT_ARRAY safearray or stream 
    {_T("boolean"), DT_BOOLEAN},    // boolean, "1" or "0" VT_BOOL int 
    {_T("char"), DT_CHAR},          // char, string VT_UI2 wchar 
    {_T("date"), DT_DATE_ISO8601},  // date.iso8601, A date in ISO 8601 format. (no time) VT_DATE long 
    {_T("datetime"), DT_DATETIME_ISO8601},      // dateTime.iso8601, A date in ISO 8601 format, with optional time and no optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
    {_T("datetime.tz"), DT_DATETIME_ISO8601TZ}, // dateTime.iso8601tz, A date in ISO 8601 format, with optional time and optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
    {_T("entities"), DT_AV_ENTITIES},   // AV_ENTITIES   = 6,
    {_T("entity"), DT_AV_ENTITY},   // AV_ENTITY     = 5,
    {_T("enumeration"), DT_AV_ENUMERATION}, // AV_ENUMERATION= 10,
    {_T("fixed.14.4"), DT_FIXED_14_4},          // fixed.14.4, Same as "number" but no more than 14 digits to the left of the decimal point, and no more than 4 to the right. VT_CY large_integer 
    {_T("float"), DT_FLOAT},        // float, Same as for "number." VT_R8 double 
    {_T("float.ieee.754.32"), DT_FLOAT_IEEE_754_32},    // float.IEEE.754.32, " VT_FLOAT float 
    {_T("float.ieee.754.64"), DT_FLOAT_IEEE_754_64},    // float.IEEE.754.64, " VT_DOUBLE double 
    {_T("i1"), DT_I1},              // i1, A number, with optional sign, no fractions, no exponent. VT_I1 char 
    {_T("i2"), DT_I2},              // i2, " VT_I2 short 
    {_T("i4"), DT_I4},              // i4, " VT_I4 long
    {_T("id"), DT_AV_ID},           // AV_ID         = 2,
    {_T("idref"), DT_AV_IDREF},     // AV_IDREF      = 3,
    {_T("idrefs"), DT_AV_IDREFS},   // AV_IDREFS     = 4,
    {_T("int"), DT_INT},            // int, A number, with optional sign, no fractions, no exponent. VT_I4 long 
    {_T("nmtoken"), DT_AV_NMTOKEN},     // AV_NMTOKEN    = 7,
    {_T("nmtokens"), DT_AV_NMTOKENS},   // AV_NMTOKENS   = 8,
    {_T("notation"), DT_AV_NOTATION},   // AV_NOTATION   = 9,
    {_T("number"), DT_NUMBER},      // number, A number, with no limit on digits, may potentially have a leading sign, fractional digits, and optionally an exponent. Punctuation as in US English. VT_R8 double 
    {_T("r4"), DT_R4},              // r4, Same as "number." VT_FLOAT float 
    {_T("r8"), DT_R8},              // r8, " VT_DOUBLE double 
    {_T("string"), DT_STRING},      // string, pcdata VT_BSTR BSTR 
    {_T("time"), DT_TIME_ISO8601},  // time.iso8601, A time in ISO 8601 format, with no date and no time zone. VT_DATE long 
    {_T("time.tz"), DT_TIME_ISO8601TZ}, // time.iso8601.tz, A time in ISO 8601 format, with no date but optional time zone. VT_DATE. long 
    {_T("ui1"), DT_UI1},            // ui1, A number, unsigned, no fractions, no exponent. VT_UI1 unsigned char 
    {_T("ui2"), DT_UI2},            // ui2, " VT_UI2 unsigned short 
    {_T("ui4"), DT_UI4},            // ui4, " VT_UI4 unsigned long
    {_T("uri"), DT_URI},            // uri, Universal Resource Identifier VT_BSTR BSTR 
    {_T("uuid"), DT_UUID},          // uuid, Hexadecimal digits representing octets, optional embedded hyphens which should be ignored. VT_BSTR GUID 
};

// -Since we don't have to worry about localization, this is _much_ faster than calling out
// to any of the standard comparison functions.
// - if maxlen < 0, then that implies that the pwchB is null terminated
static
int
_fastcmp(const TCHAR * pwszA, const TCHAR * pwchB, int maxlen)
{
    TCHAR chA, chB;

Loop:
    if (maxlen--)
    {
        chA = *pwszA++;
        chB = *pwchB++;
        if (0!=chA || 0!=chB)
        {
            if (chA==chB)
                goto Loop;
            if (chA<chB)
                return -1; // pwchA is shorter/less
            else
                return  1; // pwchB is shorter
        }
    }

    // is pwszA is longer, this isn't really a match!
    if (0 != *pwszA)
        return 1;
    return 0;
}


DataType
LookupDataType(String * psName, bool fThrowError)
{
    String * pDTName = psName->trim();
    int strlen = pDTName->length();
    // empty means DT_NONE
    if (0==strlen)
        return DT_NONE;

    // all names are matched in lowercase
    pDTName = pDTName->toLowerCase();

    // store these so we don't recalculate them over and over
    const TCHAR * pchSrc = pDTName->getWCHARPtr();

    // because, from here on, we only use pchSrc, we need to
    // addref pDTName to make sure it is not GCd.  This is
    // safe (w/o a try/catch) because none of these operations
    // throw exceptions
    pDTName->AddRef();

    DataType dt = DT_USER_DEFINED;

    // basic b-search
    const DTNAME * pDTNAME = s_adtinfo;
    int size = sizeof(s_adtinfo)/sizeof(DTNAME);
    int mid;
    int cmp;

Loop:
    mid = size/2;
    cmp = _fastcmp(pDTNAME[mid].pszName, pchSrc, strlen);
    // match?
    if (0 == cmp)
    {
        dt = pDTNAME[mid].dt;
        goto Done;
    }
    // total failure?
    if (0 == mid)
        goto Done;  // default 'dt' is error
    if (0 > cmp)
    {
        // adjust for the fact that we just looked at the first
        // item in this region...
        mid++;
        pDTNAME += mid;
        size -= mid;
    }
    else
        size = mid;
    goto Loop;

Done:
    pDTName->Release();
    if (fThrowError && DT_USER_DEFINED == dt)
        Exception::throwE(E_FAIL, XMLOM_INVALID_DATATYPE, psName, NULL);
    return dt;
}

const TCHAR *
LookupDataTypeName(DataType dt)
{
    Assert(((ULONG)dt) <= (sizeof(pwszDataTypeNames)/sizeof(TCHAR *)));
    return pwszDataTypeNames[(ULONG)dt];
}


const WCHAR *
SkipWhiteSpace(const WCHAR * pS, int cch)
{
    while (cch-- && isWhiteSpace(*pS))
        pS++;

    return pS;
}


/**
 * Lookup table, datatype -> VT_ store, parse flags
 */
static const struct DTPARSE
{
    VARTYPE         vt;
    DWORD           dwParse;
    DWORD           dwVtBits;  
} s_adtparse[DT_USER_DEFINED] = 
{
    { VT_BSTR,          // DT_NONE
        0, 
        0 },
    { VT_BSTR,          // DT_STRING & DT_AV_CDATA
        0, 
        0 },
    { VT_BSTR,          // DT_AV_ID
        0, 
        0 },
    { VT_BSTR,          // DT_AV_IDREF
        0, 
        0 },
    { VT_BSTR,          // DT_AV_IDREFS
        0, 
        0 },
    { VT_BSTR,          // DT_AV_ENTITY
        0, 
        0 },
    { VT_BSTR,          // DT_AV_ENTITIES
        0, 
        0 },
    { VT_BSTR,          // DT_AV_NMTOKEN
        0, 
        0 },
    { VT_BSTR,          // DT_AV_NMTOKENS
        0, 
        0 },
    { VT_BSTR,          // DT_AV_NOTATION
        0, 
        0 },
    { VT_BSTR,          // DT_AV_ENUMERATION
        0, 
        0 },
    { VT_BSTR,          // DT_AV_NAMEDEF
        0, 
        0 },

    { VT_ARRAY | VT_UI1,// base64
        NUMPRS_HEX_OCT, // ignored value, only to force parseNumber..
        0 },
    { VT_ARRAY | VT_UI1,// bin.hex
        NUMPRS_HEX_OCT, // ignored value, only to force parseNumber..
        0 },
    { VT_BOOL,          // boolean
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE,
        VTBIT_I2 },
    { VT_I4,           // char
        0, 
        0 },
    { VT_DATE,          // date.iso8601
        0, 
        0 },
    { VT_DATE,          // dateTime.iso8601
        0, 
        0 },
    { VT_DATE,          // dateTime.iso8601.tz
        0, 
        0 },
    { VT_CY,            // fixed.14.4
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS |
        NUMPRS_DECIMAL |
        NUMPRS_THOUSANDS,
        VTBIT_CY },
    { VT_R8,            // float
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS |
        NUMPRS_DECIMAL |
        NUMPRS_EXPONENT, 
        VTBIT_R8 },
    { VT_R4,            // float.IEEE.754.32
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS |
        NUMPRS_DECIMAL |
        NUMPRS_EXPONENT, 
        VTBIT_R4 }, 
    { VT_R8,            // float.IEEE.754.64
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS |
        NUMPRS_DECIMAL |
        NUMPRS_EXPONENT, 
        VTBIT_R8 },
    { VT_I1,            // i1
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS,
        VTBIT_I1 },
    { VT_I2,            // i2
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS, 
        VTBIT_I2 },
    { VT_I4,            // i4
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS, 
        VTBIT_I4 },
    { VT_I4,            // int
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS, 
        VTBIT_I4 }, 
    { VT_BSTR,            // number
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS |
        NUMPRS_DECIMAL |
        NUMPRS_EXPONENT, 
        VTBIT_R8 },
    { VT_R4,            // r4
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS |
        NUMPRS_DECIMAL |
        NUMPRS_EXPONENT, 
        VTBIT_R4 },
    { VT_R8,            // r8
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE |
        NUMPRS_LEADING_PLUS |
        NUMPRS_LEADING_MINUS |
        NUMPRS_DECIMAL |
        NUMPRS_EXPONENT, 
        VTBIT_R8 },
    { VT_DATE,          // time.iso8601
        0, 
        0 },
    { VT_DATE,          // time.iso8601.tz
        0, 
        0 },
    { VT_UI1,           // ui1
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE, 
        VTBIT_UI1 },
    { VT_UI2,           // ui2
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE, 
        VTBIT_UI2 },
    { VT_UI4,           // ui4
        NUMPRS_LEADING_WHITE | 
        NUMPRS_TRAILING_WHITE, 
        VTBIT_UI4 },
    { VT_BSTR,          // uri
        0, 
        0 },
    { VT_BSTR,          // uuid
        0, 
        0 },

    // ??  userdata ??

};  // struct DTPARSE {...} s_adtparse


void ConvertVariant2DataType( DataType dt, VARIANT * pVarIn, VARIANT * pVarOut)
{
    HRESULT hr;

    if (S_OK != (hr = VariantChangeTypeEx(pVarOut, pVarIn, GetSystemDefaultLCID(), VARIANT_NOVALUEPROP, VariantTypeOfDataType(dt))))
        goto Cleanup;

    switch(dt)
    {
    case DT_CHAR:
        {
            Assert(VT_I4 == pVarOut->vt);
            long l = V_I4(pVarOut);
            if ((l < 0) || (l > 0xfffd))
                hr = DISP_E_OVERFLOW;
        }
        break;
    case DT_FIXED_14_4:
        {
            Assert(VT_CY == pVarOut->vt);
            CY cy = V_CY(pVarOut);
#ifdef UNIX
            LONGLONG ll = CY_INT64(&cy);
#else
            LONGLONG ll = cy.int64;
#endif
            // BUGBUG: this is scary
            // ((ll < -123456789012341234) || (ll > 123456789012341234))
            if ((ll < -999999999999999999) || (ll > 999999999999999999))
                hr = DISP_E_OVERFLOW;
        }
        break;
    case DT_UUID:
        {
            UUID uuid;
            Assert(VT_I4 == pVarOut->vt);
            BSTR bstr = V_BSTR(pVarOut);
            hr = ParseUuid((const WCHAR *)bstr, _tcslen(bstr), &uuid, null);
        }
        break;
    }

Cleanup:
    if (hr)
    {
        VariantClear(pVarOut);
        Exception::throwE(hr);
    }
}

VARTYPE VariantTypeOfDataType( DataType dt)
{
    return s_adtparse[dt].vt;
}

/**
 * Parse passed text into specified data type
 */
HRESULT
ParseDatatype(const WCHAR * pS, int cch, 
              DataType datatype, VARIANT * pVar, 
              const WCHAR ** ppwcNext)
{
    HRESULT hr = S_OK;
    UUID uuid;
    const WCHAR * pStart = pS;
    const WCHAR * pNext = pS;
    pVar->vt = VT_NULL;
    V_BSTR(pVar) = NULL;

    if ((DT_NONE <= datatype && datatype <= DT_AV_NAMEDEF) ||
        DT_URI == datatype)
    {
        if (ppwcNext)
            *ppwcNext = pS + cch;
        // NOTE: we don't build VARIANT, since none of these
        //  require a VARIANT, no point is waisting the time 
        //  to build a BSTR.
        return S_OK;
    }

    // empty strings are fine. (but are type VT_NULL)
    if (!pS || !*(pS = SkipWhiteSpace(pS, cch)) || 0 == cch)
    {
        if (ppwcNext)
            *ppwcNext = pS;
        return S_OK;
    }

    cch -= (int)(pS - pStart);

    // if dwParse flags is set than we parse this as a number...
    if (s_adtparse[datatype].dwParse)
    {   
        hr = ParseNumeric(pS, cch, datatype, pVar, &pNext);
        if (hr)
            goto Cleanup;
    }
    else
    {
        switch(datatype)
        {
        case DT_CHAR: // char, string VT_UI2 wchar 
            pVar->vt = VT_I4;
            V_I4(pVar) = (long)*pS;
            pNext = pS + 1;
            break;
        case DT_DATE_ISO8601: // date.iso8601, A date in ISO 8601 format. (no time) VT_DATE long 
        case DT_DATETIME_ISO8601: // dateTime.iso8601, A date in ISO 8601 format, with optional time and no optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
        case DT_DATETIME_ISO8601TZ: // dateTime.iso8601tz, A date in ISO 8601 format, with optional time and optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
        case DT_TIME_ISO8601: // time.iso8601, A time in ISO 8601 format, with no date and no time zone. VT_DATE long 
        case DT_TIME_ISO8601TZ: // time.iso8601.tz, A time in ISO 8601 format, with no date but optional time zone. VT_DATE. long 
            /*  Use this for 'normal' dates later ?
            Assert(_pTypedValue->_vt == VT_EMPTY);
            hr = VarDateFromStr(const_cast<OLECHAR *>(pS), 
                getLocale(), 
                datatype < DT_DATETIME_ISO8601 ? VAR_DATEVALUEONLY : 
                    (datatype >= DT_TIME_ISO8601 ? VAR_TIMEVALUEONLY : 0 ),
                &V_DATE(&_pTypedValue->_variantData));
                */
            hr = ParseISO8601(pS, cch, 
                              (DataType)datatype, &V_DATE(pVar), 
                              &pNext);
            if (hr)
                goto Cleanup;
            pVar->vt = VT_DATE;
            break;
        case DT_NUMBER:
            {
                // these should not have whitespace in the text
                pNext = pS;
                while (cch && *pNext && !isWhiteSpace(*pNext))
                {
                    pNext++;
                    cch--;
                }
            }
            break;
        case DT_UUID: // uuid, Hexadecimal digits representing octets, optional embedded hyphens which should be ignored. VT_BSTR GUID 
            hr = ParseUuid(pS, cch, 
                           &uuid, 
                           &pNext);
            if (hr)
                goto Cleanup;
            // we loose the parsed UUID on purpose... since we have no way of exposing it.
            break;
        case DT_USER_DEFINED: // user defined user defined type VT_UNKNOWN IUnknown *
        default:
            Assert(FALSE && "don't now what to do!");
            // what to do ?
            hr = E_FAIL;
            break;
        }
    }

Cleanup:
    if (ppwcNext)
        *ppwcNext = pNext;
    else if (pNext < pS + cch)
    {
        cch -= (int)(pNext - pS);
        pS = SkipWhiteSpace(pNext, cch);
        if (pS - pNext < cch)
            // text left over !!
            hr = E_FAIL;
    }
    return hr;
}


// Take a VARIANT and build a String object.  Note, caller must
HRESULT
UnparseDatatype( String ** ppReturn, VARIANT * pVar, DataType dt)
{
    HRESULT hr = S_OK;
    VARIANT var;
    String * pString = null;
    const WCHAR * pwsz = null;
    WCHAR wch[2] = {0, 0};
    var.vt = VT_NULL;

    switch(dt)
    {
    case DT_USER_DEFINED: // user defined user defined type VT_UNKNOWN IUnknown *
        hr = E_FAIL;
        goto Cleanup;

    case DT_BASE64:
    case DT_BIN_HEX: // bin.hex, Hexadecimal digits representing octets VT_ARRAY safearray or stream
        {
            long lBound;
            BYTE * pB = null;
            hr = SafeArrayGetUBound(V_ARRAY(pVar), 1, &lBound);
            if (hr)
                goto Cleanup;
            hr = SafeArrayAccessData(V_ARRAY(pVar), (void **)&pB);
            if (hr)
                goto Cleanup;
            if (DT_BIN_HEX == dt)
                hr = UnparseBinHex( &pString, pB, lBound);
            else
                hr = UnparseBase64( pB, lBound+1, &pString);

            SafeArrayUnaccessData(V_ARRAY(pVar));
            if (hr)
                goto Cleanup;
            break;
        }

    case DT_CHAR: // char, string VT_UI2 wchar 
        wch[0] = (WCHAR)V_I4(pVar);
        pwsz = wch;
        break;

    case DT_DATE_ISO8601: // date.iso8601, A date in ISO 8601 format. (no time) VT_DATE long 
    case DT_DATETIME_ISO8601: // dateTime.iso8601, A date in ISO 8601 format, with optional time and no optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
    case DT_DATETIME_ISO8601TZ: // dateTime.iso8601tz, A date in ISO 8601 format, with optional time and optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
    case DT_TIME_ISO8601: // time.iso8601, A time in ISO 8601 format, with no date and no time zone. VT_DATE long 
    case DT_TIME_ISO8601TZ: // time.iso8601.tz, A time in ISO 8601 format, with no date but optional time zone. VT_DATE. long 
        {
            hr = UnparseISO8601( &pString, dt, &V_DATE(pVar));
            if (hr)
                goto Cleanup;
            break;
        }


    case DT_UUID: // uuid, Hexadecimal digits representing octets, optional embedded hyphens which should be ignored. VT_BSTR GUID 
        {
            BSTR bstr;
            UUID uuid;
            hr = VariantChangeTypeEx(&var, pVar, GetSystemDefaultLCID(), VARIANT_NOVALUEPROP, VT_BSTR);
            if (hr)
                goto Cleanup;

            bstr = V_BSTR(&var);
            hr = ParseUuid((const WCHAR *)bstr, _tcslen(bstr), &uuid, null);
            if (hr)
                goto Cleanup;

            pwsz = (WCHAR*)V_BSTR(&var);
            break;
        }

    case DT_BOOLEAN: // boolean, "1" or "0" VT_BOOL int 
        if (V_BOOL(pVar) == VARIANT_TRUE)
            pwsz = L"1";
        else
            pwsz = L"0";
        break;

    default:
    case DT_FIXED_14_4: // fixed.14.4, Same as "number" but no more than 14 digits to the left of the decimal point, and no more than 4 to the right. VT_CY large_integer 
    case DT_FLOAT: // float, Same as for "number." VT_R8 double 
    case DT_FLOAT_IEEE_754_32: // float.IEEE.754.32, " VT_FLOAT float 
    case DT_FLOAT_IEEE_754_64: // float.IEEE.754.64, " VT_DOUBLE double 
    case DT_I1: // i1, A number, with optional sign, no fractions, no exponent. VT_I1 char 
    case DT_I2: // i2, " VT_I2 short 
    case DT_I4: // i4, " VT_I4 long 
    case DT_INT: // int, A number, with optional sign, no fractions, no exponent. VT_I4 long 
    case DT_NUMBER: // number, A number, with no limit on digits, may potentially have a leading sign, fractional digits, and optionally an exponent. Punctuation as in US English. VT_R8 double 
    case DT_R4: // r4, Same as "number." VT_FLOAT float 
    case DT_R8: // r8, " VT_DOUBLE double 
    case DT_UI1: // ui1, A number, unsigned, no fractions, no exponent. VT_UI1 unsigned char 
    case DT_UI2: // ui2, " VT_UI2 unsigned short 
    case DT_UI4: // ui4, " VT_UI4 unsigned long
    case DT_URI: // uri, Universal Resource Identifier VT_BSTR BSTR 

    case DT_STRING: // string, pcdata VT_BSTR BSTR 
    // all the DT_AV_* types should be here also
    case DT_NONE:
        // we fall though and try and do a VarChangeTypeEx()
        break;
    }

    AWCHAR * pS;
    TRY
    {
        if (pwsz)
        {
            pString = String::newString(pwsz);
        }
        else if ( !pString)
        {
            hr = VariantChangeTypeEx(&var, pVar, GetSystemDefaultLCID(), VARIANT_NOVALUEPROP, VT_BSTR);
            if (hr)
                goto Cleanup;

            pString = String::newString( (WCHAR*)V_BSTR(&var));
        }
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY


Cleanup:
    VariantClear(&var);
    *ppReturn = pString;
    return hr;
}





static 
const WCHAR *
ParseDecimal(const WCHAR * pS, int cch,
             WORD * pw)
{
    WCHAR c;
    WORD w = 0;

    while (cch-- && isDigit(c = *pS))
    {       
        w = w * 10 + (c - _T('0'));
        pS++;
        if (w >= ((1 << 16) - 1) / 10)
            break;
    }

    *pw = w;
    return pS;
}


HRESULT
ParseBinHex( const WCHAR * pS, long lLen, 
            BYTE * abData, int * pcbSize, 
            const WCHAR ** ppwcNext)
{
    WCHAR wc;
    bool fLow = false;
    const WCHAR * pwc = pS;
    BYTE bByte;
    byte result;

    // fake a leading zero
    if ( lLen % 2 != 0)
    {
        bByte = 0;
        fLow = true;
    }

    *pcbSize = (lLen + 1) >> 1; // divide in 1/2 with round up

    // walk hex digits pairing them up and shoving the value of each pair into a byte
    while ( lLen-- > 0)
    {
        wc = *pwc++;
        if (wc >= L'a' && wc <= L'f')
        {
            result = 10 + (wc - L'a');
        }
        else if (wc >= L'A' && wc <= L'F')
        {
            result = 10 + (wc - L'A');
        }
        else if (wc >= L'0' && wc <= L'9')
        {
            result = wc - L'0';
        }
        else if (isWhiteSpace(wc))
        {
            continue; // skip whitespace
        }
        else
        {
            goto Error;
        }

        Assert((0 <= result) && (result < 16));
        if ( fLow)
        {
            bByte += (BYTE)result;
            *abData++ = bByte;
            fLow = false;
        }
        else
        {
            // shift nibble into top half of byte
            bByte = (BYTE)(result << 4);
            fLow = true;
        }   
    }

Cleanup:
    if (ppwcNext)
        *ppwcNext = pwc;
    return S_OK;

Error:
    return E_FAIL;
}


static
void swapBytes( BYTE * pb1, BYTE * pb2)
{
    BYTE t = *pb1;
    *pb1 = *pb2;
    *pb2 = t;
}


/**
 * Helper to create a char safearray from a string
 */
HRESULT
CreateVector(VARIANT * pVar, const BYTE * pData, LONG cElems)
{
    HRESULT hr;
    BYTE * pB;

    SAFEARRAY * psa = SafeArrayCreateVector(VT_UI1, 0, (unsigned int)cElems);
    if (!psa)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = SafeArrayAccessData(psa, (void **)&pB);
    if (hr)
        goto Error;

    memcpy(pB, pData, cElems);

    SafeArrayUnaccessData(psa);
    Assert((pVar->vt == VT_EMPTY) || (pVar->vt == VT_NULL));
    V_ARRAY(pVar) = psa;
    pVar->vt = VT_ARRAY | VT_UI1;

Cleanup:
    return hr;

Error:
    if (psa)
        SafeArrayDestroy(psa);
    goto Cleanup;
}

HRESULT
ParseNumber(const WCHAR * pS, int c, 
            DWORD dwParse, DWORD dwStore, VARIANT * pVar, 
            const WCHAR ** ppwcNext)
{
    HRESULT hr;
    NUMPARSE numparse;
    BYTE * abDigits =  reinterpret_cast<BYTE *>(_alloca(c));
    
    numparse.cDig = c;
    numparse.dwInFlags = dwParse;
    hr = VarParseNumFromStr(const_cast<OLECHAR *>(pS), 
        MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
        0,
        &numparse,
        abDigits);
    if (ppwcNext)
    {
        *ppwcNext = pS + numparse.cchUsed;
    }
    else if (c != numparse.cchUsed)
    {
        hr = E_FAIL;
    }
    if (hr)
        goto Cleanup;

    // BUGBUG: there should be a better way fo telling that we are parsing fixed.14.4
    if ( dwParse & NUMPRS_CURRENCY && // only happens for fixed.14.4
         ( (numparse.cDig + numparse.nPwr10 > 14) ||
           (numparse.nPwr10 < -4) ) )
    {
        // too digits to the left, or right of decimal point !!
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = VarNumFromParseNum(&numparse, 
                            abDigits,
                            dwStore,
                            pVar);
    if (hr)
        goto Cleanup;

Cleanup:

    return hr;
}


HRESULT
ParseNumeric(const WCHAR * pS, int cch, 
             DataType datatype, VARIANT * pVar, 
             const WCHAR ** ppwcNext)
{
    HRESULT hr;
    DWORD dwParse;
    DWORD dwVtBits;
    BYTE * abDigits = NULL;

    // if the caller did not supply a character count, calc it here
    if (cch == 0)
    {
        cch = _tcslen(pS);
    }

    dwVtBits = s_adtparse[datatype].dwVtBits;
    if (dwVtBits)
    {
        dwParse = s_adtparse[datatype].dwParse;
        hr = ParseNumber(pS, cch, dwParse, dwVtBits, pVar, ppwcNext);
        if (hr)
            goto Error;

        // booleans are special, we parse them as a number, but expose it as a VT_BOOL
        if ( datatype == DT_BOOLEAN)
        {
            // boolean can only have values of 0 or 1
            if ( V_I2(pVar) > 1 || V_I2(pVar) < 0)
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            pVar->vt = VT_BOOL;
            if ( V_I2(pVar) > 0)
                V_BOOL(pVar) = VARIANT_TRUE;
            else
                V_BOOL(pVar) = VARIANT_FALSE;
        }
    }
    else
    {
        int cbParsed;
        // allocate a temporary buffer based on the size of the source (in characters)
        // (this is wastefull, but safe)
        abDigits =  reinterpret_cast<BYTE *>(_alloca(cch));
        if (!abDigits)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // special handling
        Assert(DT_BIN_HEX == datatype || DT_BASE64 == datatype);
        if (DT_BIN_HEX == datatype)
            hr = ParseBinHex(pS, cch, abDigits, &cbParsed, ppwcNext);
        else if (DT_BASE64 == datatype)
            hr = ParseBase64(pS, cch, abDigits, &cbParsed, ppwcNext);
        if (hr)
            goto Cleanup;
        hr = CreateVector(pVar, abDigits, cbParsed);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    return hr;

Error:
    VariantClear( pVar);
    goto Cleanup;
}


DataType
getDateTimeType(const WCHAR * pS)
{
    // BUGBUG UNDONE
    return DT_DATE_ISO8601;
}


HRESULT
ParseDateTime(const WCHAR * pS, int cch, 
              DataType dt, VARIANT * pVar,
              const WCHAR ** ppwcNext)
{
    if (dt == DT_NONE)
    {
        dt = getDateTimeType(pS);
    }

    return ParseISO8601(pS, cch, dt, &V_DATE(pVar), ppwcNext);
}


HRESULT
ParseISO8601(const WCHAR * pS, int cch, 
             DataType dt, DATE * pdate, 
             const WCHAR ** ppwcNext)
{
    HRESULT hr = S_OK;
    UDATE udate;
    const WCHAR * pN;
    FILETIME ftTime;
    __int64 i64Offset;
    int iSign = 0;
    WORD w;
    const WCHAR * pSStart = pS;

    // make sure the enums are ordered according to logic below
    Assert(DT_DATE_ISO8601 < DT_DATETIME_ISO8601 &&
        DT_DATETIME_ISO8601 < DT_DATETIME_ISO8601TZ &&
        DT_DATETIME_ISO8601TZ < DT_TIME_ISO8601 &&
        DT_TIME_ISO8601 < DT_TIME_ISO8601TZ);

    memset(&udate, 0, sizeof(udate));
    udate.st.wMonth = 1;
    udate.st.wDay = 1;
    pS = SkipWhiteSpace(pS, cch);
    cch -= (int)(pS - pSStart);
    // parse date if allowed
    if (dt < DT_TIME_ISO8601)
    {
        pN = ParseDecimal(pS, cch, &udate.st.wYear);
        if (pN - pS != 4) // 4 digits
            goto Error;
        // HACK: VarDateFromUdate treats 2digit years specially, for y2k compliance
        if (udate.st.wYear < 100)
            goto Error;
        cch -= 4;
        pS = pN;
        if (*pS == _T('-'))
        {
            pN = ParseDecimal(pS + 1, --cch, &udate.st.wMonth);
            if (pN - pS != 3 || 0 == udate.st.wMonth || udate.st.wMonth > 12) // 2 digits + '-'
                goto Error;
            cch -= 2;
            pS = pN;
            if (*pS == _T('-'))
            {
                pN = ParseDecimal(pS + 1, --cch, &udate.st.wDay);
                if (pN - pS != 3 || 0 == udate.st.wMonth || udate.st.wDay > 31) // 2 digits + '-'
                    goto Error;
                cch -= 2;
                pS = pN;
            }
        }
        if (cch && dt >= DT_DATETIME_ISO8601)
        {
            // swallow T
            // Assume starts with T ?
            if (*pS != _T('T'))
                goto Error;
            pS++;
            cch--;
        }
    }
    else
    {
        udate.st.wYear = 1899;
        udate.st.wMonth = 12;
        udate.st.wDay = 30;
    }

    // parse time if allowed
    if (cch && dt >= DT_DATETIME_ISO8601)
    {
        pN = ParseDecimal(pS, cch, &udate.st.wHour);
        if (pN - pS != 2 || udate.st.wHour > 24) // 2 digits + 'T'
            goto Error;
        cch -= 2;
        pS = pN;
        if (*pS == _T(':'))
        {
            pN = ParseDecimal(pS + 1, --cch, &udate.st.wMinute);
            if (pN - pS != 3 || udate.st.wMinute > 59) // 2 digits + ':'
                goto Error;
            cch -= 2;
            pS = pN;
            if (*pS == _T(':'))
            {
                pN = ParseDecimal(pS + 1, --cch, &udate.st.wSecond);
                if (pN - pS != 3 || udate.st.wSecond > 59) // 2 digits + ':'
                    goto Error;
                cch -= 2;
                pS = pN;
                if (*pS == _T('.'))
                {
                    pN = ParseDecimal(pS + 1, --cch, &udate.st.wMilliseconds);
                    int d = (int)(3 - (pN - (pS + 1)));
                    if (d > 2) // at least 1 digit
                        goto Error;
                    cch -= (int)(pN - pS - 1);
                    pS = pN;
                    while (d > 0)
                    {
                        udate.st.wMilliseconds *= 10;
                        d--;
                    }
                    while (d < 0)
                    {
                        udate.st.wMilliseconds /= 10;
                        d++;
                    }
                }
            }
        }

        // watch for 24:01... etc
        if (udate.st.wHour == 24 && (udate.st.wMinute > 0 || udate.st.wSecond > 0 || udate.st.wMilliseconds > 0))
            goto Error;

        // parse timezone if allowed
        if (cch && (dt == DT_DATETIME_ISO8601TZ || dt == DT_TIME_ISO8601TZ))
        {
            if (*pS == _T('+'))
                iSign = 1;
            else if (*pS == _T('-'))
                iSign = -1;
            else if (*pS == _T('Z'))
            {
                pS++;
                cch--;
            }
            if (iSign)
            {
                if (!SystemTimeToFileTime(&udate.st, &ftTime))
                    goto Error;
                pN = ParseDecimal(pS + 1, --cch, &w);
                if (pN - pS != 3) // 2 digits + '+' or '-'
                    goto Error;
                i64Offset = (__int64)w;
                cch -= 2;
                pS = pN;
                if (*pS != ':')
                    goto Error;
                pN = ParseDecimal(pS + 1, --cch, &w);
                if (pN - pS != 3) // 2 digits + ':'
                    goto Error;
                cch -= 2;
                pS = pN;
                // convert to 100 nanoseconds
                i64Offset = 10000000 * 60 * ((__int64)w + 60 * i64Offset);
                *(__int64 *)&ftTime = *(__int64 *)&ftTime + iSign * i64Offset;
                if (!FileTimeToSystemTime(&ftTime, &udate.st))
                    goto Error;
            }
        }
    }

    hr = VarDateFromUdate(&udate, 0, pdate);
    if (hr)
        goto Cleanup;

Cleanup:
    if (ppwcNext)
        *ppwcNext = pS;

    return hr;

Error:
    hr = E_FAIL;
    goto Cleanup;
}


// example
// 81E76750-CE81-11d1-B1C8-00C04F983E60
HRESULT
ParseUuid(const WCHAR * pS, int cch, 
          UUID * pUuid, 
          const WCHAR ** ppwcNext)
{
    HRESULT hr = S_OK;
    long l;
    UUID uuid;
    int cbSize;

    // scan pS looking for the end
    const WCHAR * pScan = pS;
    while (cch-- && *pScan && !isWhiteSpace(*pScan))
        pScan++;
    l = (long)(pScan - pS);

    // if string is of the wrong length, or is missing separators, abort
    if ( l != 36 || 
         pS[8] != L'-' ||
         pS[13] != L'-' ||
         pS[18] != L'-' ||
         pS[23] != L'-')
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = ParseBinHex( const_cast<WCHAR *>(pS),  8, (BYTE *)&uuid.Data1, &cbSize, null);
    if (hr)
        goto Cleanup;
    Assert( cbSize == sizeof(unsigned long));
    // fixup ordering
    swapBytes( (BYTE *)&uuid.Data1, ((BYTE *)&uuid.Data1)+3);
    swapBytes( ((BYTE *)&uuid.Data1)+1, ((BYTE *)&uuid.Data1)+2);

    hr = ParseBinHex( const_cast<WCHAR *>(pS+9), 4, (BYTE *)&uuid.Data2, &cbSize, null);
    if (hr)
        goto Cleanup;
    Assert( cbSize == sizeof(unsigned short));
    // fixup ordering
    swapBytes( (BYTE *)&uuid.Data2, ((BYTE *)&uuid.Data2)+1);

    hr = ParseBinHex( const_cast<WCHAR *>(pS+14), 4, (BYTE *)&uuid.Data3, &cbSize, null);
    if (hr)
        goto Cleanup;
    Assert( cbSize == sizeof(unsigned short));
    // fixup ordering
    swapBytes( (BYTE *)&uuid.Data3, ((BYTE *)&uuid.Data3)+1);

    hr = ParseBinHex( const_cast<WCHAR *>(pS+19), 4, (BYTE *)&uuid.Data4, &cbSize, null);
    if (hr)
        goto Cleanup;
    Assert( cbSize == (sizeof(unsigned char) * 2));

    hr = ParseBinHex( const_cast<WCHAR *>(pS+24), 12, ((BYTE *)&uuid.Data4)+2, &cbSize, null);
    if (hr)
        goto Cleanup;
    Assert( cbSize == (sizeof(unsigned char) * 6));

    *pUuid = uuid;
    // done
    
Cleanup:
    if (ppwcNext)
        *ppwcNext = pScan;

    return hr;
}


HRESULT
UnparseDecimal( StringBuffer * pSBuffer, WORD num, long digits)
{
    HRESULT hr = S_OK;
    unsigned short digit;
    unsigned short place = 1;
    // since num is WORD == 16bits, we are better off useing ushort-s above...
    if ( digits > 5)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    // start with the most significant digit
    while( --digits) place *= 10;

    // for each digit, pull the digit out of num and store it in the string buffer
    while ( place > 0)
    {
        digit = num/place;
        if (digit > 9)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        pSBuffer->append( (WCHAR)('0'+digit));
        num -= digit*place;
        place /= 10;
    }
Cleanup:
    return hr;
}


HRESULT
UnparseISO8601( String ** ppReturn, DataType dt, DATE * pdate)
{
    HRESULT hr = S_OK;
    UDATE udate;
    StringBuffer * pSBuffer;
    
    TRY
    {
        pSBuffer = StringBuffer::newStringBuffer();

        memset(&udate, 0, sizeof(udate));
        hr = VarUdateFromDate(*pdate, 0, &udate);
        if (hr)
            goto Cleanup;

        // parse date if allowed
        if (dt < DT_TIME_ISO8601)
        {
            if ((hr = UnparseDecimal(pSBuffer, udate.st.wYear, 4)) != S_OK)
                goto Cleanup;
            pSBuffer->append(_T('-'));

            if ((hr = UnparseDecimal(pSBuffer, udate.st.wMonth, 2)) != S_OK)
                goto Cleanup;
            pSBuffer->append(_T('-'));

            if ((hr = UnparseDecimal(pSBuffer, udate.st.wDay, 2)) != S_OK)
                goto Cleanup;

            if (dt >= DT_DATETIME_ISO8601)
            {
                // starts with T
                pSBuffer->append(_T('T'));
            }
        }

        // parse time if allowed
        if (dt >= DT_DATETIME_ISO8601)
        {
            if ((hr = UnparseDecimal(pSBuffer, udate.st.wHour, 2)) != S_OK)
                goto Cleanup;
            pSBuffer->append(_T(':'));

            if ((hr = UnparseDecimal(pSBuffer, udate.st.wMinute, 2)) != S_OK)
                goto Cleanup;
            pSBuffer->append(_T(':'));

            if ((hr = UnparseDecimal(pSBuffer, udate.st.wSecond, 2)) != S_OK)
                goto Cleanup;
            pSBuffer->append(_T('.'));

            if ((hr = UnparseDecimal(pSBuffer, udate.st.wMilliseconds, 3)) != S_OK)
                goto Cleanup;
        }

        //if (dt == DT_DATETIME_ISO8601TZ || dt == DT_TIME_ISO8601TZ)
        // no time zone...

        *ppReturn = String::newString( pSBuffer);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

Cleanup:
    if (hr)
        *ppReturn = null;
    return hr;

Error:
    hr = E_FAIL;
    goto Cleanup;
}


HRESULT
UnparseBinHex( String ** ppReturn, BYTE * abData, long lLen)
{
    WCHAR * pText;
    WCHAR * pwc;
    BYTE * pb = abData;
    BYTE nibble;
    long lStrLen = lLen * 2;
    HRESULT hr = S_OK;

    pText = new_ne WCHAR[lStrLen];
    if ( !pText)
    {
        return E_OUTOFMEMORY;
    }

    pwc = pText;
    while ( lLen--)
    {
        // high nibble
        nibble = ((0xf0) & *pb) >> 4;
        if ( nibble > 9)
            *pwc++ = L'a' + nibble - 10;
        else
            *pwc++ = L'0' + nibble;
        // low nibble
        nibble = (0x0f) & *pb;
        if ( nibble > 9)
            *pwc++ = L'a' + nibble - 10;
        else
            *pwc++ = L'0' + nibble;
        // next byte...
        pb++;
    }

    TRY
    {
        *ppReturn = String::newString( pText, 0, lStrLen);
    }
    CATCH
    {
        hr = ERESULT;
        *ppReturn = null;
    }
    ENDTRY


Cleanup:
    delete pText;
    return hr;
}

//==============================================================================
//MOVED to xmlstream.cxx so that parsertest.cxx can link them in also.
//HRESULT HexToUnicode(const WCHAR* text, ULONG len, WCHAR& ch);
//HRESULT DecimalToUnicode(const WCHAR* text, ULONG len, WCHAR& ch);

//==============================================================================
//==============================================================================
// Base64 code from MTS code sample (SimpleLog)

// These characters are the legal digits, in order, that are 
// used in Base64 encoding 
// 
static 
const WCHAR rgwchBase64[] = 
    L"ABCDEFGHIJKLMNOPQ" 
    L"RSTUVWXYZabcdefgh" 
    L"ijklmnopqrstuvwxy" 
    L"z0123456789+/"; 

HRESULT
UnparseBase64(void * pvData, int cbData, String ** ppReturn) 
{ 
    int cb = cbData; 
    *ppReturn = NULL; 
    HRESULT hr = S_OK; 
    int cchPerLine = 72;                        // conservative, must be mult of 4 for us 
    int cbPerLine  = cchPerLine / 4 * 3; 
    int cbSafe     = cb + 3;                    // allow for padding 
    int cLine      = cbSafe / cbPerLine + 2;    // conservative 
    int cchNeeded  = cLine * (cchPerLine + 2 /*CRLF*/) + 1 /*NULL*/; 
    int cbNeeded   = cchNeeded * sizeof(WCHAR); 
    WCHAR * wsz = new_ne WCHAR[cbNeeded];
    BYTE*  pb   = (BYTE*)pvData; 
    WCHAR* pch  = wsz; 
    int cchLine = 0; 

    if (!wsz)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // 
        // Main encoding loop 
        // 
        while (cb >= 3) 
        { 
            BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
            BYTE b1 = ((pb[0]&0x03)<<4) | ((pb[1]>>4) & 0x0F); 
            BYTE b2 = ((pb[1]&0x0F)<<2) | ((pb[2]>>6) & 0x03); 
            BYTE b3 = ((pb[2]&0x3F)); 

            *pch++ = rgwchBase64[b0]; 
            *pch++ = rgwchBase64[b1]; 
            *pch++ = rgwchBase64[b2]; 
            *pch++ = rgwchBase64[b3]; 

            pb += 3; 
            cb -= 3; 
         
            // put in line breaks 
            cchLine += 4; 
            if (cchLine >= cchPerLine) 
            { 
                *pch++ = L'\r'; 
                *pch++ = L'\n'; 
                cchLine = 0; 
            } 
        } 
        // 
        // Account for gunk at the end 
        // 
        if ((cchLine+4) >= cchPerLine)
        {
            *pch++ = L'\r';     // easier than keeping track
            *pch++ = L'\n';
        }

        if (cb==0) 
        { 
            // nothing to do 
        } 
        else if (cb==1) 
        { 
            BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
            BYTE b1 = ((pb[0]&0x03)<<4) | 0; 
            *pch++ = rgwchBase64[b0]; 
            *pch++ = rgwchBase64[b1]; 
            *pch++ = L'='; 
            *pch++ = L'='; 
        } 
        else if (cb==2) 
        { 
            BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
            BYTE b1 = ((pb[0]&0x03)<<4) | ((pb[1]>>4) & 0x0F); 
            BYTE b2 = ((pb[1]&0x0F)<<2) | 0; 
            *pch++ = rgwchBase64[b0]; 
            *pch++ = rgwchBase64[b1]; 
            *pch++ = rgwchBase64[b2]; 
            *pch++ = L'='; 
        } 
     
        // 
        // NULL terminate the string 
        // 
        *pch = NULL; 

        // 
        // Allocate our final output 
        // 
        *ppReturn = String::newString(wsz);

        #ifdef _DEBUG 
        if (hr==S_OK) 
        { 
            int cb; void * pv = new BYTE[cbData+1];
            Assert(S_OK == ParseBase64(wsz, (LONG)(pch - wsz), pv, &cb, null));
            Assert(cb == cbData); 
            Assert(memcmp(pv, pvData, cbData) == 0); 
            delete [] (BYTE *)pv; 
        } 
        #endif

        delete [] wsz;
    }

Cleanup:
    return hr;
} 
 
 
HRESULT
ParseBase64(const WCHAR * pwc, int  cch, 
            void * pvData, int * pcbData, 
            const WCHAR ** ppwcNext)
{ 
    HRESULT hr = S_OK;
    BYTE* rgb = (BYTE *)pvData;

    BYTE  mpwchb[256]; 
    BYTE  bBad = (BYTE)-1; 
    BYTE  i; 

    if ( !rgb)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // 
        // Initialize our decoding array 
        // 
        memset(&mpwchb[0], bBad, 256); 
        for (i = 0; i < 64; i++) 
        { 
            WCHAR wch = rgwchBase64[i]; 
            mpwchb[wch] = i; 
        } 

        // 
        // Loop over the entire input buffer 
        // 
        ULONG bCurrent = 0;         // what we're in the process of filling up 
        int  cbitFilled = 0;        // how many bits in it we've filled 
        BYTE* pb = rgb;             // current destination (not filled)
        const WCHAR* pwch;
        WCHAR wch;
        // 
        for (pwch=pwc; (wch = *pwch) && cch--; pwch++) 
        { 
            // 
            // Ignore white space 
            // 
            if (wch==0x0A || wch==0x0D || wch==0x20 || wch==0x09) 
                continue; 
            // 
            // Have we reached the end? 
            // 
            if (wch==L'=') 
                break; 
            // 
            // How much is this character worth? 
            //
            BYTE bDigit;
            if (wch > 127 || (bDigit = mpwchb[wch]) == bBad)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            } 
            // 
            // Add in its contribution 
            // 
            bCurrent <<= 6; 
            bCurrent |= bDigit; 
            cbitFilled += 6; 
            // 
            // If we've got enough, output a byte 
            // 
            if (cbitFilled >= 8) 
            {
                ULONG b = (bCurrent >> (cbitFilled-8));     // get's top eight valid bits 
                *pb++ = (BYTE)(b&0xFF);                     // store the byte away 
                cbitFilled -= 8; 
            } 
        } 

        *pcbData = (ULONG)(pb-rgb); // length

        while (wch==L'=')
        {
            cbitFilled = 0;
            pwch++;
            wch = *pwch;
        }

        if (cbitFilled)
        {
            ULONG b = (bCurrent >> (cbitFilled-8));     // get's top eight valid bits 
            if (b)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }

        // characters used
        if (ppwcNext)
            *ppwcNext = pwch;
    }
    
Cleanup:
    return hr;
}
