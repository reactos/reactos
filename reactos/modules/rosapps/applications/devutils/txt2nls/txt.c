/*
 * PROJECT:     ReactOS TXT to NLS Converter
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/txt2nls/txt.c
 * COPYRIGHT:   Copyright 2016 Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "precomp.h"

int
txt_get_header(const char *file_path, NLS_FILE_HEADER *header)
{
    FILE *file;
    char *p;
    char buf[256];
    uint32_t line = 0;
    int res = 0;
    int found;
    uint32_t val;

    file = fopen(file_path, "r");
    if (!file)
    {
        printf("Unable to read TXT file.\n");
        return 0;
    }

    /* Find CODEPAGE entry */
    found = 0;
    while (fgets(buf, sizeof(buf), file))
    {
        ++line;

        p = strstr(buf, "CODEPAGE");
        if (p)
        {
            /* Length of CODEPAGE string is 8 chars */
            p += 8;

            /* Skip all spaces after CODEPAGE */
            while (isspace(*p)) ++p;

            /* Convert string to uint32_t */
            val = strtoul(p, &p, 10);

            /* Validate codepage value */
            if (val > 0xFFFF)
            {
                printf("Wrong codepage: %u (line: %u)\n", val, line);
                goto Cleanup;
            }

            header->CodePage = (uint16_t)val;

            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("CODEPAGE not found.\n");
        goto Cleanup;
    }

    /* Find CPINFO entry */
    found = 0;
    while (fgets(buf, sizeof(buf), file))
    {
        ++line;

        p = strstr(buf, "CPINFO");
        if (p)
        {
            /* Length of CPINFO string is 6 chars */
            p += 6;

            /* Skip all spaces after CPINFO */
            while (isspace(*p)) ++p;

            /* Convert string to uint32_t */
            val = strtoul(p, &p, 10);

            /* Validate value */
            if (val != 1 && val != 2)
            {
                printf("Wrong character size: %u (line: %u)\n", val, line);
                goto Cleanup;
            }

            header->MaximumCharacterSize = (uint16_t)val;

            /* Skip all spaces after character size */
            while (isspace(*p)) ++p;

            /* Convert string to uint32_t */
            val = strtoul(p, &p, 16);
            header->DefaultChar = (uint16_t)val;
            /* By default set value as DefaultChar */
            header->TransDefaultChar = (uint16_t)val;

            /* Skip all spaces after default char */
            while (isspace(*p)) ++p;

            /* Convert string to uint32_t */
            val = strtoul(p, &p, 16);
            header->UniDefaultChar = (uint16_t)val;
            /* By default set value as UniDefaultChar */
            header->TransUniDefaultChar = (uint16_t)val;

            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("CPINFO not found.\n");
        goto Cleanup;
    }

    header->HeaderSize = sizeof(NLS_FILE_HEADER) / sizeof(uint16_t);

    res = 1;

Cleanup:
    fclose(file);

    return res;
}

uint16_t*
txt_get_mb_table(const char *file_path, uint16_t uni_default_char)
{
    uint16_t *table;
    char buf[256];
    char *p;
    uint32_t count = 0;
    uint32_t index;
    uint32_t line = 0;
    int found;
    int res = 0;
    FILE *file;

    table = malloc(256 * sizeof(uint16_t));
    if (!table)
    {
        printf("Memory allocation failure\n");
        return NULL;
    }

    /* Set default value for all table items */
    for (index = 0; index <= 255; index++)
        table[index] = uni_default_char;

    file = fopen(file_path, "r");
    if (!file)
    {
        printf("Unable to read TXT file.\n");
        goto Cleanup;
    }

    /* Find MBTABLE entry */
    found = 0;
    while (fgets(buf, sizeof(buf), file))
    {
        ++line;

        p = strstr(buf, "MBTABLE");
        if (p)
        {
            p += 7;

            /* Skip spaces */
            while (isspace(*p)) ++p;

            count = strtoul(p, &p, 10);
            if (count == 0 || count > 256)
            {
                printf("Wrong MBTABLE size: %u (line: %u)\n", count, line);
                goto Cleanup;
            }

            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("MBTABLE not found.\n");
        goto Cleanup;
    }

    /* Parse next line */
    while (fgets(buf, sizeof(buf), file) && count)
    {
        uint32_t cp_char;
        uint32_t uni_char;

        ++line;

        p = buf;

        /* Skip spaces */
        while (isspace(*p)) ++p;

        if (!*p || p[0] == ';')
            continue;

        cp_char = strtoul(p, &p, 16);
        if (cp_char > 0xFF)
        {
            printf("Wrong char value: %u (line: %u)\n", cp_char, line);
            goto Cleanup;
        }

        /* Skip spaces */
        while (isspace(*p)) ++p;

        uni_char = strtoul(p, &p, 16);
        if (uni_char > 0xFFFF)
        {
            printf("Wrong unicode char value: %u (line: %u)\n", uni_char, line);
            goto Cleanup;
        }

        table[cp_char] = uni_char;
        --count;
    }

    res = 1;

Cleanup:
    if (!res)
    {
        free(table);
        table = NULL;
    }

    fclose(file);

    return table;
}

uint16_t*
txt_get_wc_table(const char *file_path, uint16_t default_char, int is_dbcs)
{
    char buf[256];
    char *p;
    uint16_t *table;
    uint32_t index;
    uint32_t count = 0;
    uint32_t line = 0;
    int res = 0;
    int found;
    FILE *file;

    table = malloc(65536 * (is_dbcs ? sizeof(uint16_t) : sizeof(uint8_t)));
    if (!table)
    {
        printf("Memory allocation failure\n");
        return NULL;
    }

    /* Set default value for all table items */
    for (index = 0; index <= 65535; index++)
    {
        /* DBCS code page */
        if (is_dbcs)
        {
            uint16_t *tmp = (uint16_t*)table;
            tmp[index] = default_char;
        }
        /* SBCS code page */
        else
        {
            uint8_t *tmp = (uint8_t*)table;
            tmp[index] = default_char;
        }
    }

    file = fopen(file_path, "r");
    if (!file)
    {
        printf("Unable to read TXT file.\n");
        goto Cleanup;
    }

    /* Find WCTABLE entry */
    found = 0;
    while (fgets(buf, sizeof(buf), file))
    {
        ++line;

        p = strstr(buf, "WCTABLE");
        if (p)
        {
            p += 7;

            /* Skip spaces */
            while (isspace(*p)) ++p;

            count = strtoul(p, &p, 10);
            if (count == 0 || count > 65536)
            {
                printf("Wrong WCTABLE size: %u (line: %u)\n", count, line);
                goto Cleanup;
            }

            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("WCTABLE not found.\n");
        goto Cleanup;
    }

    /* Parse next line */
    while (fgets(buf, sizeof(buf), file) && count)
    {
        uint32_t cp_char;
        uint32_t uni_char;

        ++line;

        p = buf;

        /* Skip spaces */
        while (isspace(*p)) ++p;

        if (!*p || p[0] == ';')
            continue;

        uni_char = strtoul(p, &p, 16);
        if (uni_char > 0xFFFF)
        {
            printf("Wrong unicode char value: %u (line: %u)\n", uni_char, line);
            goto Cleanup;
        }

        /* Skip spaces */
        while (isspace(*p)) ++p;

        cp_char = strtoul(p, &p, 16);
        if ((is_dbcs && cp_char > 0xFFFF) || (!is_dbcs && cp_char > 0xFF))
        {
            printf("Wrong char value: %u (line: %u)\n", cp_char, line);
            goto Cleanup;
        }

        /* DBCS code page */
        if (is_dbcs)
        {
            uint16_t *tmp = (uint16_t*)table;
            tmp[uni_char] = cp_char;
        }
        /* SBCS code page */
        else
        {
            uint8_t *tmp = (uint8_t*)table;
            tmp[uni_char] = cp_char;
        }

        --count;
    }

    res = 1;

Cleanup:
    if (!res)
    {
        free(table);
        table = NULL;
    }

    fclose(file);

    return table;
}

uint16_t*
txt_get_glyph_table(const char *file_path, uint16_t uni_default_char)
{
    uint16_t *table;
    char buf[256];
    char *p;
    uint32_t count = 0;
    uint32_t index;
    uint32_t line = 0;
    int found;
    int res = 0;
    FILE *file;

    table = malloc(256 * sizeof(uint16_t));
    if (!table)
    {
        printf("Memory allocation failure\n");
        return NULL;
    }

    /* Set default value for all table items */
    for (index = 0; index <= 255; index++)
        table[index] = uni_default_char;

    file = fopen(file_path, "r");
    if (!file)
    {
        printf("Unable to read TXT file.\n");
        goto Cleanup;
    }

    /* Find GLYPHTABLE entry */
    found = 0;
    while (fgets(buf, sizeof(buf), file))
    {
        ++line;

        p = strstr(buf, "GLYPHTABLE");
        if (p)
        {
            p += 10;

            /* Skip spaces */
            while (isspace(*p)) ++p;

            count = strtoul(p, &p, 10);
            if (count == 0 || count > 256)
            {
                printf("Wrong GLYPHTABLE size: %u (line: %u)\n", count, line);
                goto Cleanup;
            }

            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("GLYPHTABLE not found.\n");
        goto Cleanup;
    }

    /* Parse next line */
    while (fgets(buf, sizeof(buf), file) && count)
    {
        uint32_t cp_char;
        uint32_t uni_char;

        ++line;

        p = buf;

        /* Skip spaces */
        while (isspace(*p)) ++p;

        if (!*p || p[0] == ';')
            continue;

        cp_char = strtoul(p, &p, 16);
        if (cp_char > 0xFF)
        {
            printf("Wrong char value: %u (line: %u)\n", cp_char, line);
            goto Cleanup;
        }

        /* Skip spaces */
        while (isspace(*p)) ++p;

        uni_char = strtoul(p, &p, 16);
        if (uni_char > 0xFFFF)
        {
            printf("Wrong unicode char value: %u (line: %u)\n", uni_char, line);
            goto Cleanup;
        }

        table[cp_char] = uni_char;
        --count;
    }

    res = 1;

Cleanup:
    if (!res)
    {
        free(table);
        table = NULL;
    }

    fclose(file);

    return table;
}
