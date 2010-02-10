/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Image functions for symbol info
 */

#ifndef __L2L_IMAGE_H__
#define __L2L_IMAGE_H__

#include <rsym.h>

size_t fixup_offset(size_t ImageBase, size_t offset);

PROSSYM_ENTRY find_offset(void *data, size_t offset);

PIMAGE_SECTION_HEADER get_sectionheader(const void *FileData);

int get_ImageBase(char *fname, size_t *ImageBase);


#endif /* __L2L_IMAGE_H__ */

/* EOF */
