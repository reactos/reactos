#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "dwarf.h"
#include "pe.h"

/* Adapted for PE */

Dwarf*
dwarfopen(Pe *pe)
{
	Dwarf *d;

	if(pe == nil){
		werrstr("nil pe passed to dwarfopen");
		return nil;
	}

	d = mallocz(sizeof(Dwarf), 1);
	if(d == nil)
		return nil;

	d->pe = pe;
	if(pe->loadsection(pe, ".debug_abbrev", &d->abbrev) < 0
	|| pe->loadsection(pe, ".debug_aranges", &d->aranges) < 0
	|| pe->loadsection(pe, ".debug_line", &d->line) < 0
	|| pe->loadsection(pe, ".debug_pubnames", &d->pubnames) < 0
	|| pe->loadsection(pe, ".debug_info", &d->info) < 0)
		goto err;
	pe->loadsection(pe, ".debug_frame", &d->frame);
	pe->loadsection(pe, ".debug_ranges", &d->ranges);
	pe->loadsection(pe, ".debug_str", &d->str);

	return d;

err:
	DPRINT("Failed to open dwarf\n");
	free(d->abbrev.data);
	free(d->aranges.data);
	free(d->frame.data);
	free(d->line.data);
	free(d->pubnames.data);
	free(d->ranges.data);
	free(d->str.data);
	free(d->info.data);
	free(d);
	return nil;
}

void
dwarfclose(Dwarf *d)
{
	free(d->abbrev.data);
	free(d->aranges.data);
	free(d->frame.data);
	free(d->line.data);
	free(d->pubnames.data);
	free(d->ranges.data);
	free(d->str.data);
	free(d->info.data);
	pefree(d->pe);
	free(d);
}

