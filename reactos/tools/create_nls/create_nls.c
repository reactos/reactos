/*
 * Tool for creating NT-like NLS files for Unicode <-> Codepage conversions.
 * Tool for creating NT-like l_intl.nls file for case mapping of unicode
 * characters.
 * Copyright 2000 Timoshkov Dmitry
 * Copyright 2001 Matei Alexandru
 *
 * Sources of information:
 * Andrew Kozin's YAW project http://www.chat.ru/~stanson/yaw_en.html
 * Ove Kхven's investigations http://www.ping.uio.no/~ovehk/nls
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

static const WCHAR * const uprtable[256];
static const WCHAR * const lwrtable[256];

#define NLSDIR "../../media/nls"
#define LIBDIR "unicode.org/"

typedef struct {
    WORD wSize; /* in words 0x000D */
    WORD CodePage;
    WORD MaxCharSize; /* 1 or 2 */
    BYTE DefaultChar[MAX_DEFAULTCHAR];
    WCHAR UnicodeDefaultChar;
    WCHAR unknown1;
    WCHAR unknown2;
    BYTE LeadByte[MAX_LEADBYTES];
} __attribute__((packed)) NLS_FILE_HEADER;

/*
Support for translation from the multiple unicode chars
to the single code page char.

002D;HYPHEN-MINUS;Pd;0;ET;;;;;N;;;;;
00AD;SOFT HYPHEN;Pd;0;ON;;;;;N;;;;;
2010;HYPHEN;Pd;0;ON;;;;;N;;;;;
2011;NON-BREAKING HYPHEN;Pd;0;ON;<noBreak> 2010;;;;N;;;;;
2013;EN DASH;Pd;0;ON;;;;;N;;;;;
2014;EM DASH;Pd;0;ON;;;;;N;;;;;
2015;HORIZONTAL BAR;Pd;0;ON;;;;;N;QUOTATION DASH;;;;
*/

/* HYPHEN-MINUS aliases */
static WCHAR hyphen_aliases[] = {0x00AD,0x2010,0x2011,0x2013,0x2014,0x2015,0};

static struct {
    WCHAR cp_char;
    WCHAR *alias; /* must be 0 terminated */
} u2cp_alias[] = {
/* HYPHEN-MINUS aliases */
{0x002D, hyphen_aliases}
};

static void patch_aliases(void *u2cp, CPINFOEXA *cpi)
{
    int i, j;
    WCHAR *wc, *alias;
    BYTE *c;

    if(cpi->MaxCharSize == 2) {
	wc = (WCHAR *)u2cp;
	for(i = 0; i < 65536; i++) {
	    for(j = 0; j < sizeof(u2cp_alias)/sizeof(u2cp_alias[0]); j++) {
		alias = u2cp_alias[j].alias;
		while(*alias) {
		    if(*alias == i && wc[i] == *(WCHAR *)cpi->DefaultChar) {
			wc[i] = u2cp_alias[j].cp_char;
		    }
		    alias++;
		}
	    }
	}
    }
    else {
	c = (BYTE *)u2cp;
	for(i = 0; i < 65536; i++) {
	    for(j = 0; j < sizeof(u2cp_alias)/sizeof(u2cp_alias[0]); j++) {
		alias = u2cp_alias[j].alias;
		while(*alias) {
		    if(*alias == i && c[i] == cpi->DefaultChar[0] && u2cp_alias[j].cp_char < 256) {
			c[i] = (BYTE)u2cp_alias[j].cp_char;
		    }
		    alias++;
		}
	    }
	}
    }
}

static BOOL write_unicode2cp_table(FILE *out, CPINFOEXA *cpi, WCHAR *table)
{
    void *u2cp;
    WCHAR *wc;
    CHAR *c;
    int i;
    BOOL ret = TRUE;

    u2cp = malloc(cpi->MaxCharSize * 65536);
    if(!u2cp) {
	printf("Not enough memory for Unicode to Codepage table\n");
	return FALSE;
    }

    if(cpi->MaxCharSize == 2) {
	wc = (WCHAR *)u2cp;
	for(i = 0; i < 65536; i++)
	    wc[i] = *(WCHAR *)cpi->DefaultChar;

	for(i = 0; i < 65536; i++)
	    if (table[i] != '?')
		wc[table[i]] = (WCHAR)i;
    }
    else {
	c = (CHAR *)u2cp;
	for(i = 0; i < 65536; i++)
	    c[i] = cpi->DefaultChar[0];

	for(i = 0; i < 256; i++)
	    if (table[i] != '?')
		c[table[i]] = (CHAR)i;
    }

    patch_aliases(u2cp, cpi);

    if(fwrite(u2cp, 1, cpi->MaxCharSize * 65536, out) != cpi->MaxCharSize * 65536)
	ret = FALSE;

    free(u2cp);

    return ret;
}

static BOOL write_lb_ranges(FILE *out, CPINFOEXA *cpi, WCHAR *table)
{
    WCHAR sub_table[256];
    WORD offset, offsets[256];
    int i, j, range;

    memset(offsets, 0, sizeof(offsets));

    offset = 0;

    for(i = 0; i < MAX_LEADBYTES; i += 2) {
	for(range = cpi->LeadByte[i]; range != 0 && range <= cpi->LeadByte[i + 1]; range++) {
	    offset += 256;
	    offsets[range] = offset;
	}
    }

    if(fwrite(offsets, 1, sizeof(offsets), out) != sizeof(offsets))
	return FALSE;

    for(i = 0; i < MAX_LEADBYTES; i += 2) {
	for(range = cpi->LeadByte[i]; range != 0 && range <= cpi->LeadByte[i + 1]; range++) {
	    /*printf("Writing sub table for LeadByte %02X\n", range);*/
	    for(j = MAKEWORD(0, range); j <= MAKEWORD(0xFF, range); j++) {
		sub_table[j - MAKEWORD(0, range)] = table[j];
	    }

	    if(fwrite(sub_table, 1, sizeof(sub_table), out) != sizeof(sub_table))
		return FALSE;
	}
    }

    return TRUE;
}

static BOOL create_nls_file(char *name, CPINFOEXA *cpi, WCHAR *table, WCHAR *oemtable)
{
    FILE *out;
    NLS_FILE_HEADER nls;
    WORD wValue, number_of_lb_ranges, number_of_lb_subtables, i;

    printf("Creating NLS table \"%s\"\n", name);

    if(!(out = fopen(name, "wb"))) {
	printf("Could not create file \"%s\"\n", name);
	return FALSE;
    }

    memset(&nls, 0, sizeof(nls));

    nls.wSize = sizeof(nls) / sizeof(WORD);
    nls.CodePage = cpi->CodePage;
    nls.MaxCharSize = cpi->MaxCharSize;
    memcpy(nls.DefaultChar, cpi->DefaultChar, MAX_DEFAULTCHAR);
    nls.UnicodeDefaultChar = cpi->UnicodeDefaultChar;
    nls.unknown1 = '?';
    nls.unknown2 = '?';
    memcpy(nls.LeadByte, cpi->LeadByte, MAX_LEADBYTES);

    if(fwrite(&nls, 1, sizeof(nls), out) != sizeof(nls)) {
	fclose(out);
	printf("Could not write to file \"%s\"\n", name);
	return FALSE;
    }

    number_of_lb_ranges = 0;
    number_of_lb_subtables = 0;

    for(i = 0; i < MAX_LEADBYTES; i += 2) {
	if(cpi->LeadByte[i] != 0 && cpi->LeadByte[i + 1] > cpi->LeadByte[i]) {
	    number_of_lb_ranges++;
	    number_of_lb_subtables += cpi->LeadByte[i + 1] - cpi->LeadByte[i] + 1;
	}
    }

    /*printf("Number of LeadByte ranges %d\n", number_of_lb_ranges);*/
    /*printf("Number of LeadByte subtables %d\n", number_of_lb_subtables);*/

    /* Calculate offset to Unicode to CP table in words:
     *  1. (256 * sizeof(WORD)) primary CP to Unicode table +
     *  2. (WORD) optional OEM glyph table size in words +
     *  3. OEM glyph table size in words * sizeof(WORD) +
     *  4. (WORD) Number of DBCS LeadByte ranges + 
     *  5. if (Number of DBCS LeadByte ranges != 0) 256 * sizeof(WORD) offsets of lead byte sub tables
     *  6. (Number of DBCS LeadByte sub tables * 256 * sizeof(WORD)) LeadByte sub tables +
     *  7. (WORD) Unknown flag
     */

    wValue = (256 * sizeof(WORD) + /* 1 */
	      sizeof(WORD) + /* 2 */
	      ((oemtable !=NULL) ? (256 * sizeof(WORD)) : 0) + /* 3 */
	      sizeof(WORD) + /* 4 */
	      ((number_of_lb_subtables != 0) ? 256 * sizeof(WORD) : 0) + /* 5 */
	      number_of_lb_subtables * 256 * sizeof(WORD) + /* 6 */
	      sizeof(WORD) /* 7 */
	      ) / sizeof(WORD);

    /* offset of Unicode to CP table in words */
    fwrite(&wValue, 1, sizeof(wValue), out);

    /* primary CP to Unicode table */
    if(fwrite(table, 1, 256 * sizeof(WCHAR), out) != 256 * sizeof(WCHAR)) {
	fclose(out);
	printf("Could not write to file \"%s\"\n", name);
	return FALSE;
    }

    /* optional OEM glyph table size in words */
    wValue = (oemtable != NULL) ? (256 * sizeof(WORD)) : 0;
    fwrite(&wValue, 1, sizeof(wValue), out);

    /* optional OEM to Unicode table */
    if (oemtable) {
	if(fwrite(oemtable, 1, 256 * sizeof(WCHAR), out) != 256 * sizeof(WCHAR)) {
	    fclose(out);
	    printf("Could not write to file \"%s\"\n", name);
	    return FALSE;
	}
    }

    /* Number of DBCS LeadByte ranges */
    fwrite(&number_of_lb_ranges, 1, sizeof(number_of_lb_ranges), out);

    /* offsets of lead byte sub tables and lead byte sub tables */
    if(number_of_lb_ranges > 0) {
	if(!write_lb_ranges(out, cpi, table)) {
	    fclose(out);
	    printf("Could not write to file \"%s\"\n", name);
	    return FALSE;
	}
    }

    /* Unknown flag */
    wValue = 0;
    fwrite(&wValue, 1, sizeof(wValue), out);

    if(!write_unicode2cp_table(out, cpi, table)) {
	fclose(out);
	printf("Could not write to file \"%s\"\n", name);
	return FALSE;
    }

    fclose(out);
    return TRUE;
}

/* correct the codepage information such as default chars */
static void patch_codepage_info(CPINFOEXA *cpi)
{
    /* currently nothing */
}

static WCHAR *Load_CP2Unicode_Table(char *table_name, UINT cp, CPINFOEXA *cpi)
{
    char buf[256];
    char *p;
    DWORD n, value;
    FILE *file;
    WCHAR *table;
    int lb_ranges, lb_range_started, line;

    printf("Loading translation table \"%s\"\n", table_name);
    
    /* Init to default values */
    memset(cpi, 0, sizeof(CPINFOEXA));
    cpi->CodePage = cp;
    *(WCHAR *)cpi->DefaultChar = '?';
    cpi->MaxCharSize = 1;
    cpi->UnicodeDefaultChar = '?';

    patch_codepage_info(cpi);

    table = (WCHAR *)malloc(sizeof(WCHAR) * 65536);
    if(!table) {
	printf("Not enough memory for Codepage to Unicode table\n");
	return NULL;
    }

    for(n = 0; n < 256; n++)
	table[n] = (WCHAR)n;

    for(n = 256; n < 65536; n++)
	table[n] = cpi->UnicodeDefaultChar;

    file = fopen(table_name, "r");
    if(file == NULL) {
	free(table);
	return NULL;
    }

    line = 0;
    lb_ranges = 0;
    lb_range_started = 0;

    while(fgets(buf, sizeof(buf), file)) {
	line++;
	p = buf;
	while(isspace(*p)) p++;

	if(!*p || p[0] == '#')
	    continue;

	n = strtol(p, &p, 0);
	if(n > 0xFFFF) {
	    printf("Line %d: Entry 0x%06lX: File \"%s\" corrupted\n", line, n, table_name);
	    continue;
	}

	if(n > 0xFF && cpi->MaxCharSize != 2) {
	    /*printf("Line %d: Entry 0x%04lX: Switching to DBCS\n", line, n);*/
	    cpi->MaxCharSize = 2;
	}

	while(isspace(*p)) p++;

	if(!*p || p[0] == '#') {
	    /*printf("Line %d: Entry 0x%02lX has no Unicode value\n", line, n);*/
	}
	else {
	    value = strtol(p, &p, 0);
	    if(value > 0xFFFF) {
		printf("Line %d: Entry 0x%06lX unicode value: File \"%s\" corrupted\n", line, n, table_name);
	    }
	    table[n] = (WCHAR)value;
	}

	/* wait for comment */
	while(*p && *p != '#') p++;

	if(*p == '#' && strstr(p, "DBCS LEAD BYTE")) {
	    /*printf("Line %d, entry 0x%02lX DBCS LEAD BYTE\n", line, n);*/
	    if(n > 0xFF) {
		printf("Line %d: Entry 0x%04lX: Error: DBCS lead byte overflowed\n", line, n);
		continue;
	    }

	    table[n] = (WCHAR)0;

	    if(lb_range_started) {
		cpi->LeadByte[(lb_ranges - 1) * 2 + 1] = (BYTE)n;
	    }
	    else {
		/*printf("Line %d: Starting new DBCS lead byte range, entry 0x%02lX\n", line, n);*/
		if(lb_ranges < MAX_LEADBYTES/2) {
		    lb_ranges++;
		    lb_range_started = 1;
		    cpi->LeadByte[(lb_ranges - 1) * 2] = (BYTE)n;
		}
		else
		    printf("Line %d: Error: could not start new lead byte range\n", line);
	    }
	}
	else {
	    if(lb_range_started)
		lb_range_started = 0;
	}
    }

    fclose(file);

    return table;
}

static WCHAR *Load_OEM2Unicode_Table(char *table_name, WCHAR *def_table, UINT cp, CPINFOEXA *cpi)
{
    char buf[256];
    char *p;
    DWORD n, value;
    FILE *file;
    WCHAR *table;
    int line;

    printf("Loading oem glyph table \"%s\"\n", table_name);
    
    table = (WCHAR *)malloc(sizeof(WCHAR) * 65536);
    if(!table) {
	printf("Not enough memory for Codepage to Unicode table\n");
	return NULL;
    }

    memcpy(table, def_table, 65536 * sizeof(WCHAR));

    file = fopen(table_name, "r");
    if(file == NULL) {
	free(table);
	return NULL;
    }

    while(fgets(buf, sizeof(buf), file)) {
	line++;
	p = buf;
	while(isspace(*p)) p++;

	if(!*p || p[0] == '#')
	    continue;

	value = strtol(p, &p, 16);
	if(value > 0xFFFF) {
	    printf("Line %d: Entry 0x%06lX: File \"%s\" corrupted\n", line, value, table_name);
	    continue;
	}

	while(isspace(*p)) p++;

	if(!*p || p[0] == '#') {
	    /*printf("Line %d: Entry 0x%02lX has no Unicode value\n", line, n);*/
	    continue;
	}
	else {
	    n = strtol(p, &p, 16);
	    if(n > 0xFFFF) {
		printf("Line %d: Entry 0x%06lX unicode value: File \"%s\" corrupted\n", line, value, table_name);
		continue;
	    }
	}

	if (cpi->CodePage == 864) {
	    while(isspace(*p)) p++;

	    if(!*p || p[0] == '#' || p[0] == '-') {
		/*printf("Line %d: Entry 0x%02lX has no Unicode value\n", line, n);*/
		continue;
	    }
	    else {
		n = strtol(p, &p, 16);
		if(n > 0xFFFF) {
		    printf("Line %d: Entry 0x%06lX oem value: File \"%s\" corrupted\n", line, value, table_name);
		}
		continue;
	    }
	}

	table[n] = (WCHAR)value;
    }

    fclose(file);

    return table;
}

int write_nls_files()
{
    WCHAR *table;
    WCHAR *oemtable;
    char nls_filename[256];
    CPINFOEXA cpi;
    int i;
    struct code_page {
	UINT cp;
	BOOL oem;
	char *table_filename;
	char *comment;
    } pages[] = {
	{37,  FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/EBCDIC/CP037.TXT", "IBM EBCDIC US Canada"},
	{424, FALSE, LIBDIR"MAPPINGS/VENDORS/MISC/CP424.TXT", "IBM EBCDIC Hebrew"},
	{437, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP437.TXT", "OEM United States"},
	{500, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/EBCDIC/CP500.TXT", "IBM EBCDIC International"},
	/*{708, FALSE, "", "Arabic ASMO"},*/
	/*{720, FALSE, "", "Arabic Transparent ASMO"},*/
	{737, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP737.TXT", "OEM Greek 437G"},
	{775, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP775.TXT", "OEM Baltic"},
	{850, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP850.TXT", "OEM Multilingual Latin 1"},
	{852, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP852.TXT", "OEM Slovak Latin 2"},
	{855, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP855.TXT", "OEM Cyrillic" },
	{856, TRUE,  LIBDIR"MAPPINGS/VENDORS/MISC/CP856.TXT", "Hebrew PC"},
	{857, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP857.TXT", "OEM Turkish"},
	{860, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP860.TXT", "OEM Portuguese"},
	{861, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP861.TXT", "OEM Icelandic"},
	{862, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP862.TXT", "OEM Hebrew"},
	{863, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP863.TXT", "OEM Canadian French"},
	{864, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP864.TXT", "OEM Arabic"},
	{865, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP865.TXT", "OEM Nordic"},
	{866, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP866.TXT", "OEM Russian"},
	{869, TRUE,  LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP869.TXT", "OEM Greek"},
	/*{870, FALSE, "", "IBM EBCDIC Multilingual/ROECE (Latin 2)"},*/
	{874, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/PC/CP874.TXT", "ANSI/OEM Thai"},
	{875, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/EBCDIC/CP875.TXT", "IBM EBCDIC Greek"},
	{878, FALSE, LIBDIR"MAPPINGS/VENDORS/MISC/KOI8-R.TXT", "Russian KOI8"},
	{932, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP932.TXT", "ANSI/OEM Japanese Shift-JIS"},
	{936, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP936.TXT", "ANSI/OEM Simplified Chinese GBK"},
	{949, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP949.TXT", "ANSI/OEM Korean Unified Hangul"},
	{950, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP950.TXT", "ANSI/OEM Traditional Chinese Big5"},
	{1006, FALSE, LIBDIR"MAPPINGS/VENDORS/MISC/CP1006.TXT", "IBM Arabic"},
	{1026, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/EBCDIC/CP1026.TXT", "IBM EBCDIC Latin 5 Turkish"},
	{1250, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1250.TXT", "ANSI Eastern Europe"},
	{1251, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1251.TXT", "ANSI Cyrillic"},
	{1252, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1252.TXT", "ANSI Latin 1"},
	{1253, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1253.TXT", "ANSI Greek"},
	{1254, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1254.TXT", "ANSI Turkish"},
	{1255, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1255.TXT", "ANSI Hebrew"},
	{1256, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1256.TXT", "ANSI Arabic"},
	{1257, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1257.TXT", "ANSI Baltic"},
	{1258, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1258.TXT", "ANSI/OEM Viet Nam"},
	{10000, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/MAC/ROMAN.TXT", "Mac Roman"},
	{10006, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/MAC/GREEK.TXT", "Mac Greek"},
	{10007, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/MAC/CYRILLIC.TXT", "Mac Cyrillic"},
	{10029, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/MAC/LATIN2.TXT", "Mac Latin 2"},
	{10079, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/MAC/ICELAND.TXT", "Mac Icelandic"},
	{10081, FALSE, LIBDIR"MAPPINGS/VENDORS/MICSFT/MAC/TURKISH.TXT", "Mac Turkish"},
	/*{20000, FALSE, "", "CNS Taiwan"},*/
	/*{20001, FALSE, "", "TCA Taiwan"},*/
	/*{20002, FALSE, "", "Eten Taiwan"},*/
	/*{20003, FALSE, "", "IBM5550 Taiwan"},*/
	/*{20004, FALSE, "", "TeleText Taiwan"},*/
	/*{20005, FALSE, "", "Wang Taiwan"},*/
	/*{20105, FALSE, "", "IA5 IRV International Alphabet No.5"},*/
	/*{20106, FALSE, "", "IA5 German"},*/
	/*{20107, FALSE, "", "IA5 Swedish"},*/
	/*{20108, FALSE, "", "IA5 Norwegian"},*/
	/*{20127, FALSE, "", "US ASCII"},*/
	/*{20261, FALSE, "", "T.61"},*/
	/*{20269, FALSE, "", "ISO 6937 NonSpacing Accent"},*/
	/*{20273, FALSE, "", "IBM EBCDIC Germany"},*/
	/*{20277, FALSE, "", "IBM EBCDIC Denmark/Norway"},*/
	/*{20278, FALSE, "", "IBM EBCDIC Finland/Sweden"},*/
	/*{20280, FALSE, "", "IBM EBCDIC Italy"},*/
	/*{20284, FALSE, "", "IBM EBCDIC Latin America/Spain"},*/
	/*{20285, FALSE, "", "IBM EBCDIC United Kingdom"},*/
	/*{20290, FALSE, "", "IBM EBCDIC Japanese Katakana Extended"},*/
	/*{20297, FALSE, "", "IBM EBCDIC France"},*/
	/*{20420, FALSE, "", "IBM EBCDIC Arabic"},*/
	/*{20423, FALSE, "", "IBM EBCDIC Greek"},*/
	/*{20424, FALSE, "", "IBM EBCDIC Hebrew"},*/
	/*{20833, FALSE, "", "IBM EBCDIC Korean Extended"},*/
	/*{20838, FALSE, "", "IBM EBCDIC Thai"},*/
	/*{20871, FALSE, "", "IBM EBCDIC Icelandic"},*/
	/*{20880, FALSE, "", "IBM EBCDIC Cyrillic (Russian)"},*/
	{20866, FALSE, LIBDIR"MAPPINGS/VENDORS/MISC/KOI8-R.TXT", "Russian KOI8"},
	/*{20905, FALSE, "", "IBM EBCDIC Turkish"},*/
	/*{21025, FALSE, "", "IBM EBCDIC Cyrillic (Serbian, Bulgarian)"},*/
	/*{21027, FALSE, "", "Ext Alpha Lowercase"},*/
	{28591, FALSE, LIBDIR"MAPPINGS/ISO8859/8859-1.TXT", "ISO 8859-1 Latin 1"},
	{28592, FALSE, LIBDIR"MAPPINGS/ISO8859/8859-2.TXT", "ISO 8859-2 Eastern Europe"},
	{28593, FALSE, LIBDIR"MAPPINGS/ISO8859/8859-3.TXT", "ISO 8859-3 Turkish"},
	{28594, FALSE, LIBDIR"MAPPINGS/ISO8859/8859-4.TXT", "ISO 8859-4 Baltic"},
	{28595, FALSE, LIBDIR"MAPPINGS/ISO8859/8859-5.TXT", "ISO 8859-5 Cyrillic"},
	{28596, FALSE, LIBDIR"MAPPINGS/ISO8859/8859-6.TXT", "ISO 8859-6 Arabic"},
	{28597, FALSE, LIBDIR"MAPPINGS/ISO8859/8859-7.TXT", "ISO 8859-7 Greek"},
	{28598, FALSE, LIBDIR"MAPPINGS/ISO8859/8859-8.TXT", "ISO 8859-8 Hebrew"},
	{28599, FALSE, LIBDIR"MAPPINGS/ISO8859/8859-9.TXT", "ISO 8859-9 Latin 5"}
    };

    for(i = 0; i < sizeof(pages)/sizeof(pages[0]); i++) {
	table = Load_CP2Unicode_Table(pages[i].table_filename, pages[i].cp, &cpi);
	if(!table) {
	    printf("Could not load \"%s\" (%s)\n", pages[i].table_filename, pages[i].comment);
	    continue;
	}

	if (pages[i].oem) {
	    oemtable = Load_OEM2Unicode_Table(LIBDIR"MAPPINGS/VENDORS/MISC/IBMGRAPH.TXT", table, pages[i].cp, &cpi);
	    if(!oemtable) {
		printf("Could not load \"%s\" (%s)\n", LIBDIR"MAPPINGS/VENDORS/MISC/IBMGRAPH.TXT", "IBM OEM glyph table");
		continue;
	    }
	}

	sprintf(nls_filename, "%s/c_%03d.nls", NLSDIR, cpi.CodePage);
	if(!create_nls_file(nls_filename, &cpi, table, pages[i].oem ? oemtable : NULL)) {
	    printf("Could not write \"%s\" (%s)\n", nls_filename, pages[i].comment);
	}

	if (pages[i].oem)
	    free(oemtable);

	free(table);
    }

    return 0;
}



static WORD *to_upper_org = NULL, *to_lower_org = NULL;

static WORD diffs[256];
static int number_of_diffs;

static WORD number_of_subtables_with_diffs;
/* pointers to subtables with 16 elements in each to the main table */
static WORD *subtables_with_diffs[4096];

static WORD number_of_subtables_with_offsets;
/* subtables with 16 elements  */
static WORD subtables_with_offsets[4096 * 16];

static void test_packed_table(WCHAR *table)
{
    WCHAR test_str[] = L"This is an English text. По-русски я писать умею немножко. 1234567890";
    //WORD diff, off;
    //WORD *sub_table;
    DWORD i, len;

    len = lstrlenW(test_str);

    for(i = 0; i < len + 1; i++) {
	/*off = table[HIBYTE(test_str[i])];

	sub_table = table + off;
	off = sub_table[LOBYTE(test_str[i]) >> 4];

	sub_table = table + off;
	off = LOBYTE(test_str[i]) & 0x0F;

	diff = sub_table[off];

	test_str[i] += diff;*/
	test_str[i] += table[table[table[HIBYTE(test_str[i])] + (LOBYTE(test_str[i]) >> 4)] + (LOBYTE(test_str[i]) & 0x0F)];
    }
/*
    {
	FILE *file;
	static int n = 0;
	char name[20];

	sprintf(name, "text%02d.dat", n++);
	file = fopen(name, "wb");
	fwrite(test_str, len * sizeof(WCHAR), 1, file);
	fclose(file);
    }*/
}

static BOOL CreateCaseDiff(char *table_name)
{
    char buf[256];
    char *p;
    WORD code, case_mapping;
    FILE *file;
    int line;

    to_upper_org = (WORD *)calloc(65536, sizeof(WORD));
    if(!to_upper_org) {
	printf("Not enough memory for to upper table\n");
	return FALSE;
    }

    to_lower_org = (WORD *)calloc(65536, sizeof(WORD));
    if(!to_lower_org) {
	printf("Not enough memory for to lower table\n");
	return FALSE;
    }

    file = fopen(table_name, "r");
    if(file == NULL) {
	printf("Could not open file \"%s\"\n", table_name);
	return FALSE;
    }

    line = 0;

    while(fgets(buf, sizeof(buf), file)) {
	line++;
	p = buf;
	while(*p && isspace(*p)) p++;

	if(!*p)
	    continue;

	/* 0. Code value */
	code = (WORD)strtol(p, &p, 16);

	//if(code != 0x9A0 && code != 0xBA0)
	    //continue;

	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 1. Character name */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 2. General Category */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 3. Canonical Combining Classes */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 4. Bidirectional Category */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 5. Character Decomposition Mapping */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 6. Decimal digit value */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 7. Digit value */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 8. Numeric value */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 9. Mirrored */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 10. Unicode 1.0 Name */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 11. 10646 comment field */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;

	/* 12. Uppercase Mapping */
	while(*p && isspace(*p)) p++;
	if(!*p) continue;
	if(*p != ';') {
	    case_mapping = (WORD)strtol(p, &p, 16);
	    to_upper_org[code] = case_mapping - code;
	    while(*p && *p != ';') p++;
	}
	else
	    p++;

	/* 13. Lowercase Mapping */
	while(*p && isspace(*p)) p++;
	if(!*p) continue;
	if(*p != ';') {
	    case_mapping = (WORD)strtol(p, &p, 16);
	    to_lower_org[code] = case_mapping - code;
	    while(*p && *p != ';') p++;
	}
	else
	    p++;

	/* 14. Titlecase Mapping */
	while(*p && *p != ';') p++;
	if(!*p)
	    continue;
	p++;
    }

    fclose(file);

    return TRUE;
}

static int find_diff(WORD diff)
{
    int i;

    for(i = 0; i < number_of_diffs; i++) {
	if(diffs[i] == diff)
	    return i;
    }

    return -1;
}

static WORD find_subtable_with_diffs(WORD *table, WORD *subtable)
{
    WORD index;

    for(index = 0; index < number_of_subtables_with_diffs; index++) {
	if(memcmp(subtables_with_diffs[index], subtable, 16 * sizeof(WORD)) == 0) {
	    return index;
	}
    }

    if(number_of_subtables_with_diffs >= 4096) {
	printf("Could not add new subtable with diffs, storage is full\n");
	return 0;
    }

    subtables_with_diffs[number_of_subtables_with_diffs] = subtable;
    number_of_subtables_with_diffs++;

    return index;
}

static WORD find_subtable_with_offsets(WORD *subtable)
{
    WORD index;

    for(index = 0; index < number_of_subtables_with_offsets; index++) {
	if(memcmp(&subtables_with_offsets[index * 16], subtable, 16 * sizeof(WORD)) == 0) {
	    return index;
	}
    }

    if(number_of_subtables_with_offsets >= 4096) {
	printf("Could not add new subtable with offsets, storage is full\n");
	return 0;
    }

    memcpy(&subtables_with_offsets[number_of_subtables_with_offsets * 16], subtable, 16 * sizeof(WORD));
    number_of_subtables_with_offsets++;

    return index;
}

static WORD *pack_table(WORD *table, WORD *packed_size_in_words)
{
    WORD high, low4, index;
    WORD main_index[256];
    WORD temp_subtable[16];
    WORD *packed_table;
    WORD *subtable_src, *subtable_dst;

    memset(subtables_with_diffs, 0, sizeof(subtables_with_diffs));
    number_of_subtables_with_diffs = 0;

    memset(subtables_with_offsets, 0, sizeof(subtables_with_offsets));
    number_of_subtables_with_offsets = 0;

    for(high = 0; high < 256; high++) {
	for(low4 = 0; low4 < 256; low4 += 16) {
	    index = find_subtable_with_diffs(table, &table[MAKEWORD(low4, high)]);

	    temp_subtable[low4 >> 4] = index;
	}

	index = find_subtable_with_offsets(temp_subtable);
	main_index[high] = index;
    }

    *packed_size_in_words = 0x100 + number_of_subtables_with_offsets * 16 + number_of_subtables_with_diffs * 16;
    packed_table = calloc(*packed_size_in_words, sizeof(WORD));

    /* fill main index according to the subtables_with_offsets */
    for(high = 0; high < 256; high++) {
	packed_table[high] = 0x100 + main_index[high] * 16;
    }

    //memcpy(sub_table, subtables_with_offsets, number_of_subtables_with_offsets * 16);

    /* fill subtable index according to the subtables_with_diffs */
    for(index = 0; index < number_of_subtables_with_offsets; index++) {
	subtable_dst = packed_table + 0x100 + index * 16;
	subtable_src = &subtables_with_offsets[index * 16];

	for(low4 = 0; low4 < 16; low4++) {
	    subtable_dst[low4] = 0x100 + number_of_subtables_with_offsets * 16 + subtable_src[low4] * 16;
	}
    }


    for(index = 0; index < number_of_subtables_with_diffs; index++) {
	subtable_dst = packed_table + 0x100 + number_of_subtables_with_offsets * 16 + index * 16;
	memcpy(subtable_dst, subtables_with_diffs[index], 16 * sizeof(WORD));

    }


    test_packed_table(packed_table);

    return packed_table;
}

int write_casemap_file(void)
{
    WORD packed_size_in_words, offset_to_next_table_in_words;
    WORD *packed_table, value;
    FILE *file;

    if(!CreateCaseDiff(LIBDIR"UnicodeData.txt"))
	return -1;

    file = fopen(NLSDIR"/l_intl.nls", "wb");

    /* write version number */
    value = 1;
    fwrite(&value, 1, sizeof(WORD), file);

    /* pack upper case table */
    packed_table = pack_table(to_upper_org, &packed_size_in_words);
    offset_to_next_table_in_words = packed_size_in_words + 1;
    fwrite(&offset_to_next_table_in_words, 1, sizeof(WORD), file);
    /* write packed upper case table */
    fwrite(packed_table, sizeof(WORD), packed_size_in_words, file);
    free(packed_table);

    /* pack lower case table */
    packed_table = pack_table(to_lower_org, &packed_size_in_words);
    offset_to_next_table_in_words = packed_size_in_words + 1;
    fwrite(&offset_to_next_table_in_words, 1, sizeof(WORD), file);
    /* write packed lower case table */
    fwrite(packed_table, sizeof(WORD), packed_size_in_words, file);
    free(packed_table);

    fclose(file);

    free(to_upper_org);
    free(to_lower_org);

    return 0;
}

int main()
{
	write_nls_files();
	write_casemap_file();

	return 0;
}
