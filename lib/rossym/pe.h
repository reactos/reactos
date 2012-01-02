#ifndef _LIBMACH_PE_H_
#define _LIBMACH_PE_H_

#include "compat.h"

struct DwarfBlock;
typedef struct _IMAGE_SECTION_HEADER PeSect;

typedef struct _CoffSymbol {
	ulong address;
	char *name;
} CoffSymbol;

typedef struct _Pe {
	void *fd;
	u16int (*e2)(const unsigned char *data);
	u32int (*e4)(const unsigned char *data);
	u64int (*e8)(const unsigned char *data);
	ulong imagebase, imagesize, loadbase;
	ulong nsymbols;
	CoffSymbol *symtab;
	int (*loadsection)(struct _Pe *pe, char *name, struct DwarfBlock *b);
	int nsections;
	struct _IMAGE_SECTION_HEADER *sect;
} Pe;

#define E_SYMNMLEN 8
#include <pshpack1.h>
typedef struct {
	union {
		char e_name[E_SYMNMLEN];
		struct {
			unsigned long e_zeroes;
			unsigned long e_offset;
		} e;
	} e;
	unsigned long e_value;
	short e_scnum;
	unsigned short e_type;
	unsigned char e_sclass;
	unsigned char e_numaux;
} SYMENT, *PSYMENT;
#include <poppack.h>

#define C_EXT 2
#define C_STAT 3
#define DT_FCN 0x40

Pe *peopen(const char *name);
int loaddisksection(struct _Pe *pe, char *name, struct DwarfBlock *b);
int loadmemsection(struct _Pe *pe, char *name, struct DwarfBlock *b);
u16int peget2(const unsigned char *ptr);
u32int peget4(const unsigned char *ptr);
u64int peget8(const unsigned char *ptr);
void pefree(struct _Pe *pe);
ulong pefindrva(struct _IMAGE_SECTION_HEADER *SectionHeader, int NumberOfSections, ulong TargetPhysical);
int GetStrnlen(const char *string, int maxlen);

#define ANSI_NAME_STRING(s) ((PANSI_STRING)((s)->Name))

#endif/*_LIBMACH_PE_H_*/
