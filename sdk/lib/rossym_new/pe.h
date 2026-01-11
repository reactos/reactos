#ifndef _LIBMACH_PE_H_
#define _LIBMACH_PE_H_

#include "compat.h"

struct DwarfBlock;

/*
 * Extended section header structure for 64-bit compatibility.
 * On 32-bit, ANSI_STRING fits in 8 bytes, but on 64-bit it needs 16 bytes
 * due to pointer alignment. We use a separate field for the name string.
 */
typedef struct _PeSect {
    struct _IMAGE_SECTION_HEADER hdr;
    ANSI_STRING name_str;
} PeSect;

typedef struct _Pe {
	void *fd;
	u16int (*e2)(const unsigned char *data);
	u32int (*e4)(const unsigned char *data);
	u64int (*e8)(const unsigned char *data);
	ULONG_PTR imagebase, imagesize, loadbase;
	ULONG_PTR codestart, datastart;
	int (*loadsection)(struct _Pe *pe, char *name, struct DwarfBlock *b);
	int nsections;
	PeSect *sect;
} Pe;

Pe *peopen(const char *name);
int loaddisksection(struct _Pe *pe, char *name, struct DwarfBlock *b);
int loadmemsection(struct _Pe *pe, char *name, struct DwarfBlock *b);
u16int peget2(const unsigned char *ptr);
u32int peget4(const unsigned char *ptr);
u64int peget8(const unsigned char *ptr);
void pefree(struct _Pe *pe);
ulong pefindrva(PeSect *SectionHeaders, int NumberOfSections, ulong TargetPhysical);
int GetStrnlen(const char *string, int maxlen);

#define ANSI_NAME_STRING(s) (&(s)->name_str)

#endif/*_LIBMACH_PE_H_*/
