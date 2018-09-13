//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       hisvenc.hxx
//
//  Contents:   History value encoder
//
//-------------------------------------------------------------------------

#ifndef I_HISVENC_HXX_
#define I_HISVENC_HXX_
#pragma INCMSG("--- Beg 'hisvenc.hxx'")

MtExtern(CHistValEncReader)
MtExtern(CHistValEncWriter)
MtExtern(CHisValConverter)

class CHistValEncWriter : public CEncodeWriter
{
public:
    CHistValEncWriter(CODEPAGE cp) : CEncodeWriter(cp, 256) {};
    HRESULT UnicodeToWB(TCHAR * pchIn);
};

class CHistValEncReader : public CEncodeReader
{
public:
    CHistValEncReader(CODEPAGE cp) : CEncodeReader(cp, 256) {};
    HRESULT WBToUnicode(unsigned char * pstr);
    virtual HRESULT MakeRoomForChars( int cch );
};

class CHisValConverter
{
public:
    CHisValConverter() {};
    HRESULT Convert(CStr &cstr, CODEPAGE cpFrom, CODEPAGE cpTo);
};

#pragma INCMSG("--- End 'hisvenc.hxx'")
#else
#pragma INCMSG("*** Dup 'hisvenc.hxx'")
#endif

