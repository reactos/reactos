/*
 * PROJECT:     ReactOS TXT to NLS Converter
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/txt2nls/precomp.h
 * COPYRIGHT:   Copyright 2016 Dmitry Chapyshev <dmitry@reactos.org>
 */

#ifndef __PRECOMP_H
#define __PRECOMP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>

#define LOBYTE(w) ((uint8_t)((uint32_t)(w) & 0xff))

#define MAXIMUM_LEADBYTES   12

typedef struct
{
    uint16_t HeaderSize;
    uint16_t CodePage;
    uint16_t MaximumCharacterSize;
    uint16_t DefaultChar;
    uint16_t UniDefaultChar;
    uint16_t TransDefaultChar;
    uint16_t TransUniDefaultChar;
    uint8_t LeadByte[MAXIMUM_LEADBYTES];
} NLS_FILE_HEADER;

/* nls.c */
int
nls_from_txt(const char *txt_file_path, const char *nls_file_path);

/* bestfit.c */
int
txt_get_header(const char *file_path, NLS_FILE_HEADER *header);

uint16_t*
txt_get_mb_table(const char *file_path, uint16_t uni_default_char);

uint16_t*
txt_get_wc_table(const char *file_path, uint16_t default_char, int is_dbcs);

uint16_t*
txt_get_glyph_table(const char *file_path, uint16_t uni_default_char);

#endif
