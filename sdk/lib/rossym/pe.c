#define NTOSAPI
#include <ntifs.h>
#include <ndk/ntndk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "dwarf.h"
#include "pe.h"
#include "rossympriv.h"

PeSect *pesection(Pe *pe, const char *name)
{
	int i;
	ANSI_STRING WantName;
	RtlInitAnsiString(&WantName, name);
	DPRINT("Searching for section %s\n", name);
	for (i = 0; i < pe->nsections; i++) {
		PANSI_STRING AnsiString = ANSI_NAME_STRING(&pe->sect[i]);
		if (WantName.Length == AnsiString->Length &&
			!memcmp(AnsiString->Buffer, name, WantName.Length)) {
			DPRINT("Found %s (%d) @ %x (%x)\n", name, i,
				   ((PCHAR)pe->imagebase)+pe->sect[i].VirtualAddress,
				   pe->sect[i].SizeOfRawData);
			return &pe->sect[i];
		}
	}
	DPRINT("%s not found\n", name);
	return nil;
}

u16int peget2(const unsigned char *ptr) {
	return *((u16int*)ptr);
}

u32int peget4(const unsigned char *ptr) {
	return *((u32int*)ptr);
}

u64int peget8(const unsigned char *ptr) {
	return *((u64int*)ptr);
}

int readn(void *filectx, char *buffer, ulong size) {
	return RosSymReadFile(filectx, buffer, size);
}

int seek(void *filectx, ulong position, int origin) {
	assert(origin == 0);
	return RosSymSeekFile(filectx, position);
}

static int
readblock(void *fd, DwarfBlock *b, ulong off, ulong len)
{
	b->data = malloc(len);
	if(b->data == nil)
		return -1;
	if(!seek(fd, off, 0) || !readn(fd, (char *)b->data, len)){
		free(b->data);
		b->data = nil;
		return -1;
	}
	b->len = len;
	return 0;
}

int
loaddisksection(Pe *pe, char *name, DwarfBlock *b)
{
	PeSect *s;
	if((s = pesection(pe, name)) == nil)
		return -1;
	return readblock(pe->fd, b, s->PointerToRawData, s->SizeOfRawData);
}

int
loadmemsection(Pe *pe, char *name, DwarfBlock *b)
{
	PeSect *s;

	if((s = pesection(pe, name)) == nil)
		return -1;
	DPRINT("Loading section %s (ImageBase %x RVA %x)\n", name, pe->fd, s->VirtualAddress);
	b->data = RosSymAllocMem(s->SizeOfRawData);
	b->len = s->SizeOfRawData;
	PCHAR DataSource = ((char *)pe->fd) + s->VirtualAddress;
	DPRINT("Copying to %x from %x (%x)\n", DataSource, b->data, b->len);
	RtlCopyMemory(b->data, DataSource, s->SizeOfRawData);

	return s->SizeOfRawData;
}

void *RosSymAllocMemZero(ulong size, ulong count) {
	void *res = RosSymAllocMem(size * count);
	if (res) memset(res, 0, size * count);
	return res;
}

int GetStrnlen(const char *string, int maxlen) {
	int i;
	for (i = 0; i < maxlen && string[i]; i++);
	return i;
}

void pefree(Pe *pe) {
	int i;
	for (i = 0; i < pe->nsections; i++) {
		RtlFreeAnsiString(ANSI_NAME_STRING(&pe->sect[i]));
	}
	for (i = 0; i < pe->nsymbols; i++) {
		free(pe->symtab[i].name);
	}
	free(pe->symtab);
	free(pe->sect);
	free(pe);
}

void xfree(void *v) {
	if (v) RosSymFreeMem(v);
}

ulong pefindrva(struct _IMAGE_SECTION_HEADER *SectionHeaders, int NumberOfSections, ulong TargetPhysical) {
	int i;
	DPRINT("Finding RVA for Physical %x\n", TargetPhysical);
	for (i = 0; i < NumberOfSections; i++) {
		DPRINT("Section %d name %s Raw %x Virt %x\n",
			   i,
			   ANSI_NAME_STRING(&SectionHeaders[i])->Buffer,
			   SectionHeaders[i].PointerToRawData,
			   SectionHeaders[i].VirtualAddress);
		if (TargetPhysical >= SectionHeaders[i].PointerToRawData &&
			TargetPhysical < SectionHeaders[i].PointerToRawData + SectionHeaders[i].SizeOfRawData) {
			DPRINT("RVA %x\n", TargetPhysical - SectionHeaders[i].PointerToRawData + SectionHeaders[i].VirtualAddress);
			return TargetPhysical - SectionHeaders[i].PointerToRawData + SectionHeaders[i].VirtualAddress;
		}
	}
	return nil;
}
