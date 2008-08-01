/* icdtable.h */

#ifndef OPENGL32_PRIVATE_ICDTABLE_H
#define OPENGL32_PRIVATE_ICDTABLE_H

enum icdoffsets_e
{
	ICDIDX_INVALID = -1,
#define ICD_ENTRY(x) ICDIDX_##x,
#include "icdlist.h"
#undef ICD_ENTRY
	ICDIDX_COUNT
};

typedef struct tagICDTable
{
	DWORD	num_funcs;           /*!< Normally 336 (0x150) -- the number of functions in OpenGL 1.1 */
	PROC	dispatch_table[812]; /*!< Table containing \a num_funcs pointers to OpenGL functions */
} ICDTable, *PICDTable;

#endif /* OPENGL32_PRIVATE_ICDTABLE_H */

/* EOF */
