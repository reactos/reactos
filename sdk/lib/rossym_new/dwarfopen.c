
#include <precomp.h>
#define NDEBUG
#include <debug.h>

/* Adapted for PE */

Dwarf*
dwarfopen(Pe *pe)
{
	Dwarf *d;
	int res;

	if(pe == nil){
		werrstr("nil pe passed to dwarfopen");
		return nil;
	}

	d = mallocz(sizeof(Dwarf), 1);
	if(d == nil)
		return nil;

	d->pe = pe;
	res = pe->loadsection(pe, ".debug_abbrev", &d->abbrev);
	if(res < 0) goto err;
	if(dwarfpreallocabbrev(d) < 0) goto err;

	res = pe->loadsection(pe, ".debug_aranges", &d->aranges);
	if(res < 0) goto err;

	res = pe->loadsection(pe, ".debug_line", &d->line);
	if(res < 0) goto err;

	res = pe->loadsection(pe, ".debug_info", &d->info);
	if(res < 0) goto err;

	/* .debug_loc is optional - some modules don't have it */
	pe->loadsection(pe, ".debug_loc", &d->loc);

	pe->loadsection(pe, ".debug_pubnames", &d->pubnames);
	pe->loadsection(pe, ".debug_frame", &d->frame);
	pe->loadsection(pe, ".debug_ranges", &d->ranges);
	pe->loadsection(pe, ".debug_str", &d->str);

	return d;

err:
	free(d->abbrev.data);
	free(d->aranges.data);
	free(d->frame.data);
	free(d->line.data);
	free(d->pubnames.data);
	free(d->ranges.data);
	free(d->str.data);
	free(d->info.data);
    free(d->loc.data);
	free(d->acache.buf);
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
	free(d->loc.data);
	free(d->acache.buf);
	pefree(d->pe);
	free(d);
}
