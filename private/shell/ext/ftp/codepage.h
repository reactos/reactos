//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       codepage.h
//
//  Contents:   Codepage definitions
//
//----------------------------------------------------------------------------

#ifndef _CODEPAGE_H_
#define _CODEPAGE_H_

/*
 *  Code Page Default Values.
 */
enum CODEPAGE
{
    CP_UNDEFINED    = -1L,
    CP_DEFAULT      = CP_ACP, 
    CP_US_OEM       = 437L,
    CP_852          = 852L,
    CP_THAI         = 874L,
    CP_JPN_SJ       = 932L,
    CP_CHN_GB       = 936L,
    CP_KOR_5601     = 949L,
    CP_TWN          = 950L,
    CP_UCS_2        = 1200L,
    CP_UCS_2_BE     = 1201L,
    CP_1250         = 1250L,
    CP_1251         = 1251L,
    CP_1252         = 1252L,
    CP_1253         = 1253L,
    CP_1254         = 1254L,
    CP_1255         = 1255L,
    CP_1256         = 1256L,
    CP_1257         = 1257L,
    CP_1258         = 1258L,
    CP_20127        = 20127L,
    CP_KOI8R        = 20866L,
    CP_KOI8RU       = 21866L,
    CP_ISO_8859_1   = 28591L,
    CP_ISO_8859_2   = 28592L,
    CP_ISO_8859_3   = 28593L,
    CP_ISO_8859_9   = 28599L,

    CP_AUTO         = 50001L,

    CP_ISO_2022_JP   = 50220L,
    CP_ISO_2022_JP_ESC   = 50221L,
    CP_ISO_2022_JP_SIO   = 50222L,
    CP_ISO_2022_KR   = 50225L,
    CP_ISO_2022_TW   = 50226L,
    CP_ISO_2022_CH   = 50227L,

    CP_JP_AUTO      = 50932L,
    CP_CHS_AUTO     = 50936L,
    CP_KR_AUTO      = 50949L,
    CP_CHT_AUTO     = 50950L,

    CP_CYRILLIC_AUTO = 51251L,
    CP_GREEK_AUTO   = 51253L,
    CP_ARABIC_AUTO  = 51256L,

    CP_EUC_JP       = 51932L,
    CP_EUC_CH       = 51936L,
    CP_EUC_KR       = 51949L,
    CP_EUC_TW       = 51950L,

    CP_CHN_HZ       = 52936L,

    CP_UTF_7        = 65000L,
    CP_UTF_8        = 65001L
};

enum CP_STATE
{
    INVALID_CP      = 0,
    VALID_CP          = 1,
};

#endif
