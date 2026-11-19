/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2003 Jon Griffiths
 * Copyright 2005 Dmitry Timoshkov
 * Copyright 2002, 2019 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#pragma once

typedef struct
{
    UINT   sname;                  /* 000 LOCALE_SNAME */
    UINT   sopentypelanguagetag;   /* 004 LOCALE_SOPENTYPELANGUAGETAG */
    USHORT ilanguage;              /* 008 LOCALE_ILANGUAGE */
    USHORT unique_lcid;            /* 00a unique id if lcid == 0x1000 */
    USHORT idigits;                /* 00c LOCALE_IDIGITS */
    USHORT inegnumber;             /* 00e LOCALE_INEGNUMBER */
    USHORT icurrdigits;            /* 010 LOCALE_ICURRDIGITS*/
    USHORT icurrency;              /* 012 LOCALE_ICURRENCY */
    USHORT inegcurr;               /* 014 LOCALE_INEGCURR */
    USHORT ilzero;                 /* 016 LOCALE_ILZERO */
    USHORT inotneutral;            /* 018 LOCALE_INEUTRAL (inverted) */
    USHORT ifirstdayofweek;        /* 01a LOCALE_IFIRSTDAYOFWEEK (monday=0) */
    USHORT ifirstweekofyear;       /* 01c LOCALE_IFIRSTWEEKOFYEAR */
    USHORT icountry;               /* 01e LOCALE_ICOUNTRY */
    USHORT imeasure;               /* 020 LOCALE_IMEASURE */
    USHORT idigitsubstitution;     /* 022 LOCALE_IDIGITSUBSTITUTION */
    UINT   sgrouping;              /* 024 LOCALE_SGROUPING (as binary string) */
    UINT   smongrouping;           /* 028 LOCALE_SMONGROUPING  (as binary string) */
    UINT   slist;                  /* 02c LOCALE_SLIST */
    UINT   sdecimal;               /* 030 LOCALE_SDECIMAL */
    UINT   sthousand;              /* 034 LOCALE_STHOUSAND */
    UINT   scurrency;              /* 038 LOCALE_SCURRENCY */
    UINT   smondecimalsep;         /* 03c LOCALE_SMONDECIMALSEP */
    UINT   smonthousandsep;        /* 040 LOCALE_SMONTHOUSANDSEP */
    UINT   spositivesign;          /* 044 LOCALE_SPOSITIVESIGN */
    UINT   snegativesign;          /* 048 LOCALE_SNEGATIVESIGN */
    UINT   s1159;                  /* 04c LOCALE_S1159 */
    UINT   s2359;                  /* 050 LOCALE_S2359 */
    UINT   snativedigits;          /* 054 LOCALE_SNATIVEDIGITS (array of single digits) */
    UINT   stimeformat;            /* 058 LOCALE_STIMEFORMAT (array of formats) */
    UINT   sshortdate;             /* 05c LOCALE_SSHORTDATE (array of formats) */
    UINT   slongdate;              /* 060 LOCALE_SLONGDATE (array of formats) */
    UINT   syearmonth;             /* 064 LOCALE_SYEARMONTH (array of formats) */
    UINT   sduration;              /* 068 LOCALE_SDURATION (array of formats) */
    USHORT idefaultlanguage;       /* 06c LOCALE_IDEFAULTLANGUAGE */
    USHORT idefaultansicodepage;   /* 06e LOCALE_IDEFAULTANSICODEPAGE */
    USHORT idefaultcodepage;       /* 070 LOCALE_IDEFAULTCODEPAGE */
    USHORT idefaultmaccodepage;    /* 072 LOCALE_IDEFAULTMACCODEPAGE */
    USHORT idefaultebcdiccodepage; /* 074 LOCALE_IDEFAULTEBCDICCODEPAGE */
    USHORT old_geoid;              /* 076 LOCALE_IGEOID (older version?) */
    USHORT ipapersize;             /* 078 LOCALE_IPAPERSIZE */
    BYTE   islamic_cal[2];         /* 07a calendar id for islamic calendars (?) */
    UINT   scalendartype;          /* 07c string, first char is LOCALE_ICALENDARTYPE, next chars are LOCALE_IOPTIONALCALENDAR */
    UINT   sabbrevlangname;        /* 080 LOCALE_SABBREVLANGNAME */
    UINT   siso639langname;        /* 084 LOCALE_SISO639LANGNAME */
    UINT   senglanguage;           /* 088 LOCALE_SENGLANGUAGE */
    UINT   snativelangname;        /* 08c LOCALE_SNATIVELANGNAME */
    UINT   sengcountry;            /* 090 LOCALE_SENGCOUNTRY */
    UINT   snativectryname;        /* 094 LOCALE_SNATIVECTRYNAME */
    UINT   sabbrevctryname;        /* 098 LOCALE_SABBREVCTRYNAME */
    UINT   siso3166ctryname;       /* 09c LOCALE_SISO3166CTRYNAME */
    UINT   sintlsymbol;            /* 0a0 LOCALE_SINTLSYMBOL */
    UINT   sengcurrname;           /* 0a4 LOCALE_SENGCURRNAME */
    UINT   snativecurrname;        /* 0a8 LOCALE_SNATIVECURRNAME */
    UINT   fontsignature;          /* 0ac LOCALE_FONTSIGNATURE (binary string) */
    UINT   siso639langname2;       /* 0b0 LOCALE_SISO639LANGNAME2 */
    UINT   siso3166ctryname2;      /* 0b4 LOCALE_SISO3166CTRYNAME2 */
    UINT   sparent;                /* 0b8 LOCALE_SPARENT */
    UINT   sdayname;               /* 0bc LOCALE_SDAYNAME1 (array of days 1..7) */
    UINT   sabbrevdayname;         /* 0c0 LOCALE_SABBREVDAYNAME1  (array of days 1..7) */
    UINT   smonthname;             /* 0c4 LOCALE_SMONTHNAME1 (array of months 1..13) */
    UINT   sabbrevmonthname;       /* 0c8 LOCALE_SABBREVMONTHNAME1 (array of months 1..13) */
    UINT   sgenitivemonth;         /* 0cc equivalent of LOCALE_SMONTHNAME1 for genitive months */
    UINT   sabbrevgenitivemonth;   /* 0d0 equivalent of LOCALE_SABBREVMONTHNAME1 for genitive months */
    UINT   calnames;               /* 0d4 array of calendar names */
    UINT   customsorts;            /* 0d8 array of custom sort names */
    USHORT inegativepercent;       /* 0dc LOCALE_INEGATIVEPERCENT */
    USHORT ipositivepercent;       /* 0de LOCALE_IPOSITIVEPERCENT */
    USHORT unknown1;               /* 0e0 */
    USHORT ireadinglayout;         /* 0e2 LOCALE_IREADINGLAYOUT */
    USHORT unknown2[2];            /* 0e4 */
    UINT   unused1;                /* 0e8 unused? */
    UINT   sengdisplayname;        /* 0ec LOCALE_SENGLISHDISPLAYNAME */
    UINT   snativedisplayname;     /* 0f0 LOCALE_SNATIVEDISPLAYNAME */
    UINT   spercent;               /* 0f4 LOCALE_SPERCENT */
    UINT   snan;                   /* 0f8 LOCALE_SNAN */
    UINT   sposinfinity;           /* 0fc LOCALE_SPOSINFINITY */
    UINT   sneginfinity;           /* 100 LOCALE_SNEGINFINITY */
    UINT   unused2;                /* 104 unused? */
    UINT   serastring;             /* 108 CAL_SERASTRING */
    UINT   sabbreverastring;       /* 10c CAL_SABBREVERASTRING */
    UINT   unused3;                /* 110 unused? */
    UINT   sconsolefallbackname;   /* 114 LOCALE_SCONSOLEFALLBACKNAME */
    UINT   sshorttime;             /* 118 LOCALE_SSHORTTIME (array of formats) */
    UINT   sshortestdayname;       /* 11c LOCALE_SSHORTESTDAYNAME1 (array of days 1..7) */
    UINT   unused4;                /* 120 unused? */
    UINT   ssortlocale;            /* 124 LOCALE_SSORTLOCALE */
    UINT   skeyboardstoinstall;    /* 128 LOCALE_SKEYBOARDSTOINSTALL */
    UINT   sscripts;               /* 12c LOCALE_SSCRIPTS */
    UINT   srelativelongdate;      /* 130 LOCALE_SRELATIVELONGDATE */
    UINT   igeoid;                 /* 134 LOCALE_IGEOID */
    UINT   sshortestam;            /* 138 LOCALE_SSHORTESTAM */
    UINT   sshortestpm;            /* 13c LOCALE_SSHORTESTPM */
    UINT   smonthday;              /* 140 LOCALE_SMONTHDAY (array of formats) */
    UINT   keyboard_layout;        /* 144 keyboard layouts */
} NLS_LOCALE_DATA;

typedef struct
{
    UINT   id;                     /* 00 lcid */
    USHORT idx;                    /* 04 index in locales array */
    USHORT name;                   /* 06 locale name */
} NLS_LOCALE_LCID_INDEX;

typedef struct
{
    USHORT name;                   /* 00 locale name */
    USHORT idx;                    /* 02 index in locales array */
    UINT   id;                     /* 04 lcid */
} NLS_LOCALE_LCNAME_INDEX;

typedef struct
{
    UINT   offset;                 /* 00 offset to version, always 8? */
    UINT   unknown1;               /* 04 */
    UINT   version;                /* 08 file format version */
    UINT   magic;                  /* 0c magic 'NSDS' */
    UINT   unknown2[3];            /* 10 */
    USHORT header_size;            /* 1c size of this header (?) */
    USHORT nb_lcids;               /* 1e number of lcids in index */
    USHORT nb_locales;             /* 20 number of locales in array */
    USHORT locale_size;            /* 22 size of NLS_LOCALE_DATA structure */
    UINT   locales_offset;         /* 24 offset of locales array */
    USHORT nb_lcnames;             /* 28 number of lcnames in index */
    USHORT pad;                    /* 2a */
    UINT   lcids_offset;           /* 2c offset of lcids index */
    UINT   lcnames_offset;         /* 30 offset of lcnames index */
    UINT   unknown3;               /* 34 */
    USHORT nb_calendars;           /* 38 number of calendars in array */
    USHORT calendar_size;          /* 3a size of calendar structure */
    UINT   calendars_offset;       /* 3c offset of calendars array */
    UINT   strings_offset;         /* 40 offset of strings data */
    USHORT unknown4[4];            /* 44 */
} NLS_LOCALE_HEADER;
