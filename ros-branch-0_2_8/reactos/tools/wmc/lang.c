/*
 * Wine Message Compiler language and codepage support
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wmc.h"
#include "lang.h"


/*
 * Languages supported
 *
 * MUST be sorting ascending on language ID
 */
static const language_t languages[] = {

	{0x0402, 866, 1251, "Bulgarian", "Bulgaria"},
	{0x0403, 850, 1252, "Catalan", "Spain"},
	{0x0405, 852, 1250, "Czech", "Czech Republic"},
	{0x0406, 850, 1252, "Danish", "Denmark"},
	{0x0407, 850, 1252, "German", "Germany"},
	{0x0408, 737, 1253, "Greek", "Greece"},
	{0x0409, 437, 1252, "English", "United States"},
	{0x040A, 850, 1252, "Spanish - Traditional Sort", "Spain"},
	{0x040B, 850, 1252, "Finnish", "Finland"},
	{0x040C, 850, 1252, "French", "France"},
	{0x040E, 852, 1250, "Hungarian", "Hungary"},
	{0x040F, 850, 1252, "Icelandic", "Iceland"},
	{0x0410, 850, 1252, "Italian", "Italy"},
	{0x0411, 932,  932, "Japanese", "Japan"},
	{0x0412, 949,  949, "Korean", "Korea (south)"},
	{0x0413, 850, 1252, "Dutch", "Netherlands"},
	{0x0414, 850, 1252, "Norwegian (Bokmål)", "Norway"},
	{0x0415, 852, 1250, "Polish", "Poland"},
	{0x0416, 850, 1252, "Portuguese", "Brazil"},
	{0x0418, 852, 1250, "Romanian", "Romania"},
	{0x0419, 866, 1251, "Russian", "Russia"},
	{0x041A, 852, 1250, "Croatian", "Croatia"},
	{0x041B, 852, 1250, "Slovak", "Slovakia"},
	{0x041C, 852, 1250, "Albanian", "Albania"},
	{0x041D, 850, 1252, "Swedish", "Sweden"},
	{0x041F, 857, 1254, "Turkish", "Turkey"},
	{0x0421, 850, 1252, "Indonesian", "Indonesia"},
	{0x0422, 866, 1251, "Ukrainian", "Ukraine"},
	{0x0423, 866, 1251, "Belarusian", "Belarus"},
	{0x0424, 852, 1250, "Slovene", "Slovenia"},
	{0x0425, 775, 1257, "Estonian", "Estonia"},
	{0x0426, 775, 1257, "Latvian", "Latvia"},
	{0x0427, 775, 1257, "Lithuanian", "Lithuania"},
/*	{0x042A,   ?,    ?, "Vietnamese", "Vietnam"},*/
	{0x042D, 850, 1252, "Basque", "Spain"},
	{0x042F, 866, 1251, "Macedonian", "Former Yugoslav Republic of Macedonia"},
	{0x0436, 850, 1252, "Afrikaans", "South Africa"},
/*	{0x0438, 852, 1252, "Faroese", "Faroe Islands"}, FIXME: Not sure about codepages */
	{0x043C, 437, 1252, "Irish", "Ireland"},
/*	{0x048F,   ?,    ?, "Esperanto", "<none>"},*/
/*	{0x0804,   ?,    ?, "Chinese (People's replublic of China)", People's republic of China"},*/
	{0x0807, 850, 1252, "German", "Switzerland"},
	{0x0809, 850, 1252, "English", "United Kingdom"},
	{0x080A, 850, 1252, "Spanish", "Mexico"},
	{0x080C, 850, 1252, "French", "Belgium"},
	{0x0810, 850, 1252, "Italian", "Switzerland"},
	{0x0813, 850, 1252, "Dutch", "Belgium"},
	{0x0814, 850, 1252, "Norwegian (Nynorsk)", "Norway"},
	{0x0816, 850, 1252, "Portuguese", "Portugal"},
/*	{0x081A,   ?,    ?, "Serbian (latin)", "Yugoslavia"},*/
	{0x081D, 850, 1252, "Swedish (Finland)", "Finland"},
	{0x0C07, 850, 1252, "German", "Austria"},
	{0x0C09, 850, 1252, "English", "Australia"},
	{0x0C0A, 850, 1252, "Spanish - International Sort", "Spain"},
	{0x0C0C, 850, 1252, "French", "Canada"},
	{0x0C1A, 855, 1251, "Serbian (Cyrillic)", "Serbia"},
	{0x1007, 850, 1252, "German", "Luxembourg"},
	{0x1009, 850, 1252, "English", "Canada"},
	{0x100A, 850, 1252, "Spanish", "Guatemala"},
	{0x100C, 850, 1252, "French", "Switzerland"},
	{0x1407, 850, 1252, "German", "Liechtenstein"},
	{0x1409, 850, 1252, "English", "New Zealand"},
	{0x140A, 850, 1252, "Spanish", "Costa Rica"},
	{0x140C, 850, 1252, "French", "Luxembourg"},
	{0x1809, 850, 1252, "English", "Ireland"},
	{0x180A, 850, 1252, "Spanish", "Panama"},
	{0x1C09, 437, 1252, "English", "South Africa"},
	{0x1C0A, 850, 1252, "Spanish", "Dominican Republic"},
	{0x2009, 850, 1252, "English", "Jamaica"},
	{0x200A, 850, 1252, "Spanish", "Venezuela"},
	{0x2409, 850, 1252, "English", "Caribbean"},
	{0x240A, 850, 1252, "Spanish", "Colombia"},
	{0x2809, 850, 1252, "English", "Belize"},
	{0x280A, 850, 1252, "Spanish", "Peru"},
	{0x2C09, 437, 1252, "English", "Trinidad & Tobago"},
	{0x2C0A, 850, 1252, "Spanish", "Argentina"},
	{0x300A, 850, 1252, "Spanish", "Ecuador"},
	{0x340A, 850, 1252, "Spanish", "Chile"},
	{0x380A, 850, 1252, "Spanish", "Uruguay"},
	{0x3C0A, 850, 1252, "Spanish", "Paraguay"},
	{0x400A, 850, 1252, "Spanish", "Bolivia"},
	{0x440A, 850, 1252, "Spanish", "El Salvador"},
	{0x480A, 850, 1252, "Spanish", "Honduras"},
	{0x4C0A, 850, 1252, "Spanish", "Nicaragua"},
	{0x500A, 850, 1252, "Spanish", "Puerto Rico"}
};

#define NLAN	(sizeof(languages)/sizeof(languages[0]))

void show_languages(void)
{
	int i;
	printf(" Code  | DOS-cp | WIN-cp |   Language   | Country\n");
	printf("-------+--------+--------+--------------+---------\n");
	for(i = 0; i < NLAN; i++)
		printf("0x%04x | %5d  | %5d   | %-12s | %s\n",
			languages[i].id,
			languages[i].doscp,
			languages[i].wincp,
			languages[i].name,
			languages[i].country);
}

static int langcmp(const void *p1, const void *p2)
{
	return *(unsigned *)p1 - ((language_t *)p2)->id;
}

const language_t *find_language(unsigned id)
{
	return (const language_t *)bsearch(&id, languages, NLAN, sizeof(languages[0]), langcmp);
}

void show_codepages(void)
{
#if 0
	unsigned i;
	const union cptable *cpp;
	printf("Codepages:\n");
	for(i = 0; (cpp = cp_enum_table(i)); i++)
	{
		printf("%-5d %s\n", cpp->info.codepage, cpp->info.name);
	}
#endif
}

#if 0
const union cptable *find_codepage(int id)
{
	return cp_get_table(id);
}
#endif
