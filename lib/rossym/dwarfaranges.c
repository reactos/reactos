/*
 * Dwarf address ranges parsing code.
 */

#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "dwarf.h"

int
dwarfaddrtounit(Dwarf *d, ulong addr, ulong *unit)
{
	DwarfBuf b;
	int segsize, i;
	ulong len, id, off, base, size;
	uchar *start, *end;

	memset(&b, 0, sizeof b);
	b.d = d;
	b.p = d->aranges.data;
	b.ep = b.p + d->aranges.len;

	while(b.p < b.ep){
		start = b.p;
		len = dwarfget4(&b);
		if (!len) { b.ep = b.p - 4; return -1; }
		if((id = dwarfget2(&b)) != 2){
			if(b.p == nil){
			underflow:
				werrstr("buffer underflow reading address ranges header");
			}else
				werrstr("bad dwarf version 0x%x in address ranges header", id);
			return -1;
		}
		off = dwarfget4(&b);
		b.addrsize = dwarfget1(&b);
		if(d->addrsize == 0)
			d->addrsize = b.addrsize;
		segsize = dwarfget1(&b);
		USED(segsize);	/* what am i supposed to do with this? */
		if(b.p == nil)
			goto underflow;
		if((i = (b.p-start) % (2*b.addrsize)) != 0)
			b.p += 2*b.addrsize - i;
		end = start+4+len;
		while(b.p!=nil && b.p<end){
			base = dwarfgetaddr(&b);
			size = dwarfgetaddr(&b);
			if (!size) continue;
			if(b.p == nil)
				goto underflow;
			if(base <= addr && addr < base+size){
				*unit = off;
				return 0;
			}
		}
		if(b.p == nil)
			goto underflow;
		b.p = end;
	}
	werrstr("address 0x%lux is not listed in dwarf debugging symbols", addr);
	return -1;
}


