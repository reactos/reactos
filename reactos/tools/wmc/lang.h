/*
 * Wine Message Compiler language and codepage support
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
 *
 */

#ifndef __WMC_LANG_H
#define __WMC_LANG_H

//#include "wine/unicode.h"

typedef struct language {
	unsigned	id;
	unsigned	doscp;
	unsigned	wincp;
	char		*name;
	char		*country;
} language_t;

void show_languages(void);
const language_t *find_language(unsigned id);
void show_codepages(void);
const union cptable *find_codepage(int id);

#endif
