/*
 * In place, rewrite name to compress multiple /, eliminate ., and process ..
 */

#define SEP(x)	((x)=='/' || (x) == 0)

char *
cleanname(char *name)
{
	char *p, *q, *dotdot;
	int rooted;

	rooted = name[0] == '/';

	/*
	 * invariants:
	 *	p points at beginning of path element we're considering.
	 *	q points just past the last path element we wrote (no slash).
	 *	dotdot points just past the point where .. cannot backtrack
	 *		any further (no slash).
	 */
	p = q = dotdot = name+rooted;
	while(*p) {
		if(p[0] == '/')	/* null element */
			p++;
		else if(p[0] == '.' && SEP(p[1]))
			p += 1;	/* don't count the separator in case it is nul */
		else if(p[0] == '.' && p[1] == '.' && SEP(p[2])) {
			p += 2;
			if(q > dotdot) {	/* can backtrack */
				while(--q > dotdot && *q != '/')
					;
			} else if(!rooted) {	/* /.. is / but ./../ is .. */
				if(q != name)
					*q++ = '/';
				*q++ = '.';
				*q++ = '.';
				dotdot = q;
			}
		} else {	/* real path element */
			if(q != name+rooted)
				*q++ = '/';
			while((*q = *p) != '/' && *q != 0)
				p++, q++;
		}
	}
	if(q == name)	/* empty string is really ``.'' */
		*q++ = '.';
	*q = '\0';
	return name;
}

