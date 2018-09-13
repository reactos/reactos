/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _CORE_DATATYPE_HXX
#define _CORE_DATATYPE_HXX


/**
 * Data types
 */
enum DataType
{
    DT_NONE = 0, // no datatype specified

    // like DT_NONE, only they specified it explicitly, 
    // rather than just defaulting to nothing...
    DT_STRING, // string, pcdata VT_BSTR BSTR 

    // These are DTD attribute types
    DT_AV_CDATA = DT_STRING,
    // thre rest..
    DT_AV_ID,
    DT_AV_IDREF,
    DT_AV_IDREFS,
    DT_AV_ENTITY,
    DT_AV_ENTITIES,
    DT_AV_NMTOKEN,
    DT_AV_NMTOKENS,
    DT_AV_NOTATION,
    DT_AV_ENUMERATION,
    DT_AV_NAMEDEF,

    DT__NON_AV,
    // non-SGML/DTD datatypes
    DT_BASE64 = DT__NON_AV,  // base64 as defined in the MIME IETF spec
    DT_BIN_HEX, // bin.hex, Hexadecimal digits representing octets VT_ARRAY safearray or stream 
    DT_BOOLEAN, // boolean, "1" or "0" VT_BOOL int 
    DT_CHAR, // char, string VT_UI2 wchar 
    DT_DATE_ISO8601, // date.iso8601, A date in ISO 8601 format. (no time) VT_DATE long 
    DT_DATETIME_ISO8601, // dateTime.iso8601, A date in ISO 8601 format, with optional time and no optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
    DT_DATETIME_ISO8601TZ, // dateTime.iso8601tz, A date in ISO 8601 format, with optional time and optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long 
    DT_FIXED_14_4, // fixed.14.4, Same as "number" but no more than 14 digits to the left of the decimal point, and no more than 4 to the right. VT_CY large_integer 
    DT_FLOAT, // float, Same as for "number." VT_R8 double 
    DT_FLOAT_IEEE_754_32, // float.IEEE.754.32, " VT_FLOAT float 
    DT_FLOAT_IEEE_754_64, // float.IEEE.754.64, " VT_DOUBLE double 
    DT_I1, // i1, A number, with optional sign, no fractions, no exponent. VT_I1 char 
    DT_I2, // i2, " VT_I2 short 
    DT_I4, // i4, " VT_I4 long
    DT_INT, // int, A number, with optional sign, no fractions, no exponent. VT_I4 long 
    DT_NUMBER, // number, A number, with no limit on digits, may potentially have a leading sign, fractional digits, and optionally an exponent. Punctuation as in US English. VT_R8 double 
    DT_R4, // r4, Same as "number." VT_FLOAT float 
    DT_R8, // r8, " VT_DOUBLE double 
    DT_TIME_ISO8601, // time.iso8601, A time in ISO 8601 format, with no date and no time zone. VT_DATE long 
    DT_TIME_ISO8601TZ, // time.iso8601.tz, A time in ISO 8601 format, with no date but optional time zone. VT_DATE. long 
    DT_UI1, // ui1, A number, unsigned, no fractions, no exponent. VT_UI1 unsigned char 
    DT_UI2, // ui2, " VT_UI2 unsigned short 
    DT_UI4, // ui4, " VT_UI4 unsigned long
    DT_URI, // uri, Universal Resource Identifier VT_BSTR BSTR 
    DT_UUID, // uuid, Hexadecimal digits representing octets, optional embedded hyphens which should be ignored. VT_BSTR GUID 

    DT_USER_DEFINED, // user defined user defined type VT_UNKNOWN IUnknown *
};  // enum DataType

// helpers for going from DataType Enum <-> DataType Name
const TCHAR * LookupDataTypeName(DataType dt);
DataType LookupDataType(String * psName, bool fThrowError);


class StringBuffer;
class String;

// parse text content (innerText) into the data type node
HRESULT ParseDatatype(const WCHAR * pS, int cch, 
                      DataType dt, VARIANT * pVar, 
                      const WCHAR ** ppwcNext);

// Take a VARIANT and build a String object.  
// NOTE: caller must have ppReturn pointing into the
// stack or else the GC might GC the string out from under you!
HRESULT UnparseDatatype(String ** ppReturn, 
                        VARIANT * pVar, 
                        DataType dt);

void ConvertVariant2DataType(DataType dt, 
                             VARIANT * pVarIn, 
                             VARIANT * pVarOut); // throws exceptions



// parse text content (innerText) as number into the data type
HRESULT ParseNumber(const WCHAR * pS, int cch, 
                    DWORD dwParse, DWORD dwStore, VARIANT * pVar, 
                    const WCHAR ** ppwcNext);
HRESULT ParseNumeric(const WCHAR * pS, int cch, 
                     DataType dt, VARIANT * pVar, 
                     const WCHAR ** ppwcNext);

// parse ISO date time format into VT_DATE
HRESULT ParseDateTime(const WCHAR * pS, int cch, 
                      DataType dt, VARIANT * pVar, 
                      const WCHAR ** ppwcNext);
HRESULT ParseISO8601(const WCHAR * pS, int cch, 
                     DataType dt, DATE * pDate,
                     const WCHAR ** ppwcNext);

// parse a UUID string into a UUID struct
HRESULT ParseUuid(const WCHAR * pS, int cch, 
                  UUID * pUuid, 
                  const WCHAR ** ppwcNext);

// Note: Neither of these is intended for incremental parsing, 
// since they both require a preallocated buffer.
HRESULT ParseBinHex(const WCHAR * pS, long lLen, 
                    BYTE * abData, int * pcbSize, 
                    const WCHAR ** ppwcNext);
HRESULT ParseBase64(const WCHAR * pwc, int cch, 
                    void * ppvData, int * pcbData, 
                    const WCHAR ** ppwcNext);

// build a 0 filled string of the given number of digits from the given num
HRESULT UnparseDecimal( StringBuffer * pSBuffer, WORD num, long digits);

// build an ISO conformant string from a DATE
HRESULT UnparseISO8601( String ** ppReturn, DataType dt, DATE * pdate);
// convert BinHex to string
HRESULT UnparseBinHex( String ** ppReturn, BYTE * abData, long lLen);

HRESULT UnparseBase64(void * pvData, int cbData, String ** ppReturn);



// Get the Variant Type for a DataType
VARTYPE VariantTypeOfDataType( DataType dt);

// helper functions
const WCHAR * SkipWhiteSpace(const WCHAR * pS, int cch);

HRESULT CreateVector(VARIANT * pVar, const BYTE * pData, LONG cElems);


#endif _CORE_DATATYPE_HXX
