/*
 * This software is in the public domain.
 *
 * $Id: tree.d 10510 2005-08-15 01:46:19Z kateturner $
 */

#pragma D option quiet

self int indent;
self int times[int];

php$target:::function-entry
{
	@counts[copyinstr(arg0)] = count();
        printf("%*s", self->indent, "");
        printf("-> %s\n", copyinstr(arg0));
	self->times[self->indent] = timestamp;
        self->indent += 2;
}

php$target:::function-return
{
        self->indent -= 2;
        printf("%*s", self->indent, "");
        printf("<- %s %dus\n", copyinstr(arg0), (timestamp - self->times[self->indent]) / 1000);
}
