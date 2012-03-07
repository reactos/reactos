/*
 * Dwarf data format parsing routines.
 */

#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "dwarf.h"
#include "pe.h"

ulong
dwarfget1(DwarfBuf *b)
{
	if(b->p==nil || b->p+1 > b->ep){
		b->p = nil;
		return 0;
	}
	return *b->p++;
}

int
dwarfgetn(DwarfBuf *b, uchar *a, int n)
{
	if(b->p==nil || b->p+n > b->ep){
		b->p = nil;
		memset(a, 0, n);
		return -1;
	}
	memmove(a, b->p, n);
	b->p += n;
	return 0;
}

uchar*
dwarfgetnref(DwarfBuf *b, ulong n)
{
	uchar *p;

	if(b->p==nil || b->p+n > b->ep){
		b->p = nil;
		return nil;
	}
	p = b->p;
	b->p += n;
	return p;
}

char*
dwarfgetstring(DwarfBuf *b)
{
	char *s;

	if(b->p == nil)
		return nil;
	s = (char*)b->p;
	while(b->p < b->ep && *b->p)
		b->p++;
	if(b->p >= b->ep){
		b->p = nil;
		return nil;
	}
	b->p++;
	return s;
}

void
dwarfskip(DwarfBuf *b, int n)
{
	if(b->p==nil || b->p+n > b->ep)
		b->p = nil;
	else
		b->p += n;
}

ulong
dwarfget2(DwarfBuf *b)
{
	ulong v;

	if(b->p==nil || b->p+2 > b->ep){
		b->p = nil;
		return 0;
	}
	v = b->d->pe->e2(b->p);
	b->p += 2;
	return v;
}

ulong
dwarfget4(DwarfBuf *b)
{
	ulong v;

	if(b->p==nil || b->p+4 > b->ep){
		b->p = nil;
		return 0;
	}
	v = b->d->pe->e4(b->p);
	b->p += 4;
	return v;
}

uvlong
dwarfget8(DwarfBuf *b)
{
	uvlong v;

	if(b->p==nil || b->p+8 > b->ep){
		b->p = nil;
		return 0;
	}
	v = b->d->pe->e8(b->p);
	b->p += 8;
	return v;
}

ulong
dwarfgetaddr(DwarfBuf *b)
{
	static int nbad;

	if(b->addrsize == 0)
		b->addrsize = b->d->addrsize;

	switch(b->addrsize){
	case 1:
		return dwarfget1(b);
	case 2:
		return dwarfget2(b);
	case 4:
		return dwarfget4(b);
	case 8:
		return dwarfget8(b);
	default:
		if(++nbad == 1)
			werrstr("dwarf: unexpected address size %lud in dwarfgetaddr\n", b->addrsize);
		b->p = nil;
		return 0;
	}
}

int n1, n2, n3, n4, n5;

/* An inline function picks off the calls to dwarfget128 for 1-byte encodings,
 * more than by far the common case (99.999% on most binaries!). */
ulong
dwarfget128(DwarfBuf *b)
{
	static int nbad;
	ulong c, d;

	if(b->p == nil)
		return 0;
	c = *b->p++;
	if(!(c&0x80))
{n1++;
		return c;
}
	c &= ~0x80;
	d = *b->p++;
	c |= (d&0x7F)<<7;
	if(!(d&0x80))
{n2++;
		return c;
}
	d = *b->p++;
	c |= (d&0x7F)<<14;
	if(!(d&0x80))
{n3++;
		return c;
}
	d = *b->p++;
	c |= (d&0x7F)<<21;
	if(!(d&0x80))
{n4++;
		return c;
}
	d = *b->p++;
	c |= (d&0x7F)<<28;
	if(!(d&0x80))
{n5++;
		return c;
}
	while(b->p<b->ep && *b->p&0x80)
		b->p++;
	if(++nbad == 1)
		werrstr("dwarf: overflow during parsing of uleb128 integer\n");
	return c;
}

long
dwarfget128s(DwarfBuf *b)
{
	int nb, c;
	ulong v;
	static int nbad;

	v = 0;
	nb = 0;
	if(b->p==nil)
		return 0;
	while(b->p<b->ep){
		c = *b->p++;
		v |= (c & 0x7F)<<nb;
		nb += 7;
		if(!(c&0x80))
			break;
	}
	if(v&(1<<(nb-1)))
		v |= ~(((ulong)1<<nb)-1);
	if(nb > 8*sizeof(ulong)){
		if(0)
		if(++nbad == 1)
			werrstr("dwarf: overflow during parsing of sleb128 integer: got %d bits", nb);
	}
	return v;
}


