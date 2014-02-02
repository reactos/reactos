#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "dwarf.h"

static int
_dwarfnametounit(Dwarf *d, char *name, DwarfBlock *bl, DwarfSym *s)
{
	int vers;
	ulong len, unit, off;
	uchar *next;
	char *str;
	DwarfBuf b;

	b.d = d;
	b.p = bl->data;
	b.ep = b.p + bl->len;

	while(b.p < b.ep){
		len = dwarfget4(&b);
		if(len > b.ep-b.p){
			werrstr("bad length in dwarf name header");
			return -1;
		}
		next = b.p + len;
		vers = dwarfget2(&b);
		if(vers != 1 && vers != 2){
			werrstr("bad version %d in dwarf name header", vers);
			return -1;
		}
		unit = dwarfget4(&b);
		dwarfget4(&b);	/* unit length */
		while(b.p < next){
			off = dwarfget4(&b);
			if(off == 0)
				break;
			str = dwarfgetstring(&b);
			if(strcmp(str, name) == 0){
				if(dwarfenumunit(d, unit, s) < 0)
					return -1;
				if(unit + off >= s->b.ep - d->info.data){
					werrstr("bad offset in name entry");
					return -1;
				}
				s->b.p = d->info.data + unit + off;
				if(dwarfnextsym(d, s) < 0)
					return -1;
				if(s->attrs.name==nil || strcmp(s->attrs.name, name)!=0){
					werrstr("unexpected name %#q in lookup for %#q", s->attrs.name, name);
					return -1;
				}
				return 0;
			}
		}
		b.p = next;
	}
	werrstr("unknown name '%s'", name);
	return -1;
}

int
dwarflookupname(Dwarf *d, char *name, DwarfSym *sym)
{
	return _dwarfnametounit(d, name, &d->pubnames, sym);
}

/*

int
dwarflookuptype(Dwarf *d, char *name, DwarfSym *sym)
{
	return _dwarfnametounit(d, name, &d->pubtypes, sym);
}

 */

