/*
 * Dwarf abbreviation parsing code.
 *
 * The convention here is that calling dwarfgetabbrevs relinquishes
 * access to any abbrevs returned previously.  Will have to add
 * explicit reference counting if this turns out not to be acceptable.
 */

#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "rossym.h"
#include "rossympriv.h"
#include "dwarf.h"

static int parseabbrevs(Dwarf*, ulong, DwarfAbbrev*, DwarfAttr*, int*, int*);
DwarfAbbrev *dwarfgetabbrev(Dwarf*, ulong, ulong);

static int
loadabbrevs(Dwarf *d, ulong off, DwarfAbbrev **aa)
{
	int nattr, nabbrev;
	DwarfAbbrev *abbrev;
	DwarfAttr *attr;

	if(d->acache.off == off && d->acache.na){
		*aa = d->acache.a;
		return d->acache.na;
	}

	/* two passes - once to count, then allocate, then a second to copy */
	if(parseabbrevs(d, off, nil, nil, &nabbrev, &nattr) < 0) {
		return -1;
	}

	abbrev = malloc(nabbrev*sizeof(DwarfAbbrev) + nattr*sizeof(DwarfAttr));
	attr = (DwarfAttr*)(abbrev+nabbrev);

	if(parseabbrevs(d, off, abbrev, attr, nil, nil) < 0){
		free(abbrev);
		return -1;
	}

	free(d->acache.a);
	d->acache.a = abbrev;
	d->acache.na = nabbrev;
	d->acache.off = off;

	*aa = abbrev;
	return nabbrev;
}

static int
parseabbrevs(Dwarf *d, ulong off, DwarfAbbrev *abbrev, DwarfAttr *attr, int *pnabbrev, int *pnattr)
{
	int i, nabbrev, nattr, haskids;
	ulong num, tag, name, form;
	DwarfBuf b;

	if(off >= d->abbrev.len){
		werrstr("bad abbrev section offset 0x%lux >= 0x%lux\n", off, d->abbrev.len);
		return -1;
	}

	memset(&b, 0, sizeof b);
	b.p = d->abbrev.data + off;
	b.ep = d->abbrev.data + d->abbrev.len;

	nabbrev = 0;
	nattr = 0;
	for(;;){
		if(b.p == nil){
			werrstr("malformed abbrev data");
			return -1;
		}
		num = dwarfget128(&b);
		if(num == 0)
			break;
		tag = dwarfget128(&b);
		haskids = dwarfget1(&b);
		for(i=0;; i++){
			name = dwarfget128(&b);
			form = dwarfget128(&b);
			if(name == 0 && form == 0)
				break;
			if(attr){
				attr[i].name = name;
				attr[i].form = form;
			}
		}
		if(abbrev){
			abbrev->num = num;
			abbrev->tag = tag;
			abbrev->haskids = haskids;
			abbrev->attr = attr;
			abbrev->nattr = i;
			abbrev++;
			attr += i;
		}
		nabbrev++;
		nattr += i;
	}
	if(pnabbrev)
		*pnabbrev = nabbrev;
	if(pnattr)
		*pnattr = nattr;
	return 0;
}

static DwarfAbbrev*
findabbrev(DwarfAbbrev *a, int na, ulong num)
{
	int i;

	for(i=0; i<na; i++)
		if(a[i].num == num)
			return &a[i];
	werrstr("abbrev not found");
	return nil;
}

DwarfAbbrev*
dwarfgetabbrev(Dwarf *d, ulong off, ulong num)
{
	DwarfAbbrev *a;
	int na;

	if((na = loadabbrevs(d, off, &a)) < 0){
		werrstr("loadabbrevs: %r");
		return nil;
	}
	return findabbrev(a, na, num);
}


