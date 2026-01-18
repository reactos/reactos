/*
 * Dwarf abbreviation parsing code.
 *
 * The convention here is that calling dwarfgetabbrevs relinquishes
 * access to any abbrevs returned previously.  Will have to add
 * explicit reference counting if this turns out not to be acceptable.
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

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

    if (!d->acache.buf) {
        werrstr("abbrev cache not preallocated");
        return -1;
    }
    if (nabbrev > d->acache.maxa || nattr > d->acache.maxattr) {
        werrstr("abbrev cache too small (%d/%d)", nabbrev, nattr);
        return -1;
    }

    abbrev = d->acache.buf;
    attr = d->acache.attrbuf;

    if(parseabbrevs(d, off, abbrev, attr, nil, nil) < 0){
        return -1;
    }

    d->acache.a = abbrev;
    d->acache.na = nabbrev;
    d->acache.off = off;

    *aa = abbrev;
    return nabbrev;
}

int
dwarfpreallocabbrev(Dwarf *d)
{
    DwarfBuf b;
    int max_abbrev = 0;
    int max_attr = 0;

    if (!d || d->abbrev.data == nil || d->abbrev.len == 0) {
        werrstr("missing abbrev data");
        return -1;
    }
    if (d->acache.buf)
        return 0;

    memset(&b, 0, sizeof b);
    b.d = d;
    b.p = d->abbrev.data;
    b.ep = d->abbrev.data + d->abbrev.len;

    while (b.p && b.p < b.ep) {
        int nabbrev = 0;
        int nattr = 0;

        for (;;) {
            ulong num = dwarfget128(&b);
            if (b.p == nil)
                return -1;
            if (num == 0)
                break;
            (void)dwarfget128(&b); /* tag */
            (void)dwarfget1(&b); /* haskids */

            for (;;) {
                ulong name = dwarfget128(&b);
                ulong form = dwarfget128(&b);
                if (name == 0 && form == 0)
                    break;
                nattr++;
            }
            nabbrev++;
        }

        if (nabbrev > max_abbrev)
            max_abbrev = nabbrev;
        if (nattr > max_attr)
            max_attr = nattr;

        if (b.p == nil || b.p >= b.ep)
            break;
    }

    if (max_abbrev == 0 || max_attr == 0) {
        werrstr("abbrev data empty");
        return -1;
    }

    d->acache.buf = malloc(max_abbrev*sizeof(DwarfAbbrev) +
                           max_attr*sizeof(DwarfAttr));
    if (!d->acache.buf)
        return -1;

    d->acache.attrbuf = (DwarfAttr*)(d->acache.buf + max_abbrev);
    d->acache.maxa = max_abbrev;
    d->acache.maxattr = max_attr;
    return 0;
}

static int
parseabbrevs(Dwarf *d, ulong off, DwarfAbbrev *abbrev, DwarfAttr *attr, int *pnabbrev, int *pnattr)
{
    int i, nabbrev, nattr, haskids;
    ulong num, tag, name, form;
    DwarfBuf b;

    if(off >= d->abbrev.len){
        werrstr("bad abbrev section offset 0x%lux >= 0x%lux", off, d->abbrev.len);
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
        werrstr("abbrev: num %d tag %x @ %x", num, tag, b.p - d->abbrev.data);
        haskids = dwarfget1(&b);
        for(i=0;; i++){
            name = dwarfget128(&b);
            form = dwarfget128(&b);
            assert(form < 0x3000);
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

    for(i=0; i<na; i++) {
        if(a[i].num == num) {
            return &a[i];
        }
    }
    werrstr("abbrev not found (%x)", na);
    return nil;
}

DwarfAbbrev*
dwarfgetabbrev(Dwarf *d, ulong off, ulong num)
{
    DwarfAbbrev *a;
    int na;
    werrstr("want num %d\n", num);
    if((na = loadabbrevs(d, off, &a)) < 0){
        werrstr("loadabbrevs: %r");
        return nil;
    }
    return findabbrev(a, na, num);
}
