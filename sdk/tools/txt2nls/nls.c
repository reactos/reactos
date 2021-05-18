/*
 * PROJECT:     ReactOS TXT to NLS Converter
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/txt2nls/nls.c
 * COPYRIGHT:   Copyright 2016 Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "precomp.h"

#define _NLS_DEBUG_PRINT

#ifdef _NLS_DEBUG_PRINT

static void
nls_print_header(NLS_FILE_HEADER *header)
{
    uint32_t i;

    printf("HEADER:\n");
    printf("CodePage: %u\n", header->CodePage);
    printf("Character size: %u\n", header->MaximumCharacterSize);
    printf("Default char: 0x%02X\n", header->DefaultChar);
    printf("Default unicode char: 0x%04X\n", header->UniDefaultChar);
    printf("Trans default char: 0x%02X\n", header->TransUniDefaultChar);
    printf("Trans default unicode char: 0x%04X\n", header->TransUniDefaultChar);

    for (i = 0; i < MAXIMUM_LEADBYTES; i++)
    {
        printf("LeadByte[%u] = 0x%02X\n", i, header->LeadByte[i]);
    }

    printf("\n");
}

static void
nls_print_mb_table(uint16_t *mb_table, uint16_t uni_default_char)
{
    uint32_t ch;

    printf("MBTABLE:\n");

    for (ch = 0; ch <= 0xFF; ch++)
    {
        if (mb_table[ch] != uni_default_char)
        {
            printf("0x%02X 0x%04X\n", (unsigned int)ch, (unsigned int)mb_table[ch]);
        }
    }

    printf("\n");
}

static void
nls_print_wc_table(uint16_t *wc_table, uint16_t default_char, int is_dbcs)
{
    uint32_t ch;

    printf("WCTABLE:\n");

    for (ch = 0; ch <= 0xFFFF; ch++)
    {
        /* DBCS code page */
        if (is_dbcs)
        {
            uint16_t *table = (uint16_t*)wc_table;

            if (table[ch] != default_char)
                printf("0x%04X 0x%04X\n", (unsigned int)ch, (unsigned int)table[ch]);
        }
        /* SBCS code page */
        else
        {
            uint8_t *table = (uint8_t*)wc_table;

            if (table[ch] != default_char)
                printf("0x%04X 0x%02X\n", (unsigned int)ch, (unsigned int)table[ch]);
        }
    }

    printf("\n");
}

static void
nls_print_glyph_table(uint16_t *glyph_table, uint16_t uni_default_char)
{
    uint32_t ch;

    printf("GLYPHTABLE:\n");

    for (ch = 0; ch <= 0xFF; ch++)
    {
        if (glyph_table[ch] != uni_default_char)
        {
            printf("0x%02X 0x%04X\n", (unsigned int)ch, (unsigned int)glyph_table[ch]);
        }
    }

    printf("\n");
}

#endif /* _NLS_DEBUG_PRINT */

int
nls_from_txt(const char *txt_file_path, const char *nls_file_path)
{
    NLS_FILE_HEADER header;
    FILE *file = NULL;
    uint16_t *mb_table = NULL;
    uint16_t *wc_table = NULL;
    uint16_t *glyph_table = NULL;
    uint16_t number_of_lb_ranges;
    uint16_t size;
    int is_dbcs;
    int res = 0;

    memset(&header, 0, sizeof(header));

    if (!txt_get_header(txt_file_path, &header))
        goto Cleanup;

    is_dbcs = (header.MaximumCharacterSize == 2) ? 1 : 0;

    mb_table = txt_get_mb_table(txt_file_path, header.UniDefaultChar);
    if (!mb_table)
        goto Cleanup;

    wc_table = txt_get_wc_table(txt_file_path, header.DefaultChar, is_dbcs);
    if (!wc_table)
        goto Cleanup;

    /* GLYPHTABLE optionally. We do not leave if it is absent */
    glyph_table = txt_get_glyph_table(txt_file_path, header.UniDefaultChar);

    if (is_dbcs)
    {
        /* DBCS codepage */
        uint16_t *table = (uint16_t*)wc_table;
        header.TransUniDefaultChar = table[header.UniDefaultChar];
        /* TODO: TransDefaultChar for DBCS codepages */
    }
    else
    {
        /* SBCS codepage */
        uint8_t *table = (uint8_t*)wc_table;
        header.TransUniDefaultChar = table[header.UniDefaultChar];
        header.TransDefaultChar = mb_table[LOBYTE(header.DefaultChar)];
    }

#ifdef _NLS_DEBUG_PRINT
    nls_print_header(&header);
    nls_print_mb_table(mb_table, header.UniDefaultChar);
    if (glyph_table)
        nls_print_glyph_table(glyph_table, header.UniDefaultChar);
    nls_print_wc_table(wc_table, header.DefaultChar, is_dbcs);
#endif /* _NLS_DEBUG_PRINT */

    /* Create binary file with write access */
    file = fopen(nls_file_path, "wb");
    if (!file)
    {
        printf("Unable to create NLS file.\n");
        goto Cleanup;
    }

    /* Write NLS file header */
    if (fwrite(&header, 1, sizeof(header), file) != sizeof(header))
    {
        printf("Unable to write NLS file.\n");
        goto Cleanup;
    }

    size = (256 * sizeof(uint16_t)) + /* Primary CP to Unicode table */
           sizeof(uint16_t) + /* optional OEM glyph table size in words */
           (glyph_table ? (256 * sizeof(uint16_t)) : 0) + /* OEM glyph table size in words * sizeof(uint16_t) */
           sizeof(uint16_t) + /* Number of DBCS LeadByte ranges */
           0 + /* offsets of lead byte sub tables */
           0 + /* LeadByte sub tables */
           sizeof(uint16_t); /* Unknown flag */

    size /= sizeof(uint16_t);

    if (fwrite(&size, 1, sizeof(size), file) != sizeof(size))
    {
        printf("Unable to write NLS file.\n");
        goto Cleanup;
    }

    /* Write multibyte table */
    if (fwrite(mb_table, 1, (256 * sizeof(uint16_t)), file) != (256 * sizeof(uint16_t)))
    {
        printf("Unable to write NLS file.\n");
        goto Cleanup;
    }

    /* OEM glyph table size in words */
    size = (glyph_table ? 256 : 0);

    if (fwrite(&size, 1, sizeof(size), file) != sizeof(size))
    {
        printf("Unable to write NLS file.\n");
        goto Cleanup;
    }

    if (glyph_table)
    {
        /* Write OEM glyph table */
        if (fwrite(glyph_table, 1, (256 * sizeof(uint16_t)), file) != (256 * sizeof(uint16_t)))
        {
            printf("Unable to write NLS file.\n");
            goto Cleanup;
        }
    }

    /* Number of DBCS LeadByte ranges */
    number_of_lb_ranges = 0;
    if (fwrite(&number_of_lb_ranges, 1, sizeof(number_of_lb_ranges), file) != sizeof(number_of_lb_ranges))
    {
        printf("Unable to write NLS file.\n");
        goto Cleanup;
    }

    /* Unknown flag */
    size = 0;
    if (fwrite(&size, 1, sizeof(size), file) != sizeof(size))
    {
        printf("Unable to write NLS file.\n");
        goto Cleanup;
    }

    /* Write wide char table */
    if (fwrite(wc_table, 1, (65536 * header.MaximumCharacterSize), file) != (65536 * header.MaximumCharacterSize))
    {
        printf("Unable to write NLS file.\n");
        goto Cleanup;
    }

    res = 1;

Cleanup:
    if (file) fclose(file);
    free(mb_table);
    free(wc_table);
    free(glyph_table);

    return res;
}
