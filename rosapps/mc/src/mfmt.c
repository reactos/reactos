/* mfmt: sets bold and underline for mail files */
/* (c) 1995 miguel de icaza */

#include <stdio.h>

enum states {
    header,
    definition,
    plain,
    newline,
    seen_f,
    seen_r,
    seen_o,
    header_new,
    seen_m
};

void
omain (void)
{
    enum states state = header;
    int prev = 0;
    int c;
    
    while ((c = getchar ()) != EOF){
	if (c != '\n'){
	    switch (state){ 
	    case header:
		putchar ('_');
		putchar ('\b');
		break;
		
	    case definition:
		putchar (c);
		putchar ('\b');
		break;
	    default:
		break;	/* inhibit compiler warnings */
	    }
	}
	putchar (c);
	if ((state != plain) && c == ':')
	    state = definition;

	if (c == '\n' && prev == '\n')
	    state = plain;

	if (state == definition && c == '\n')
	    state = header;
	
	prev = c;
    }
}

int
main (void)
{
    int state = newline;
    int space_seen;
    int c;
    
    while ((c = getchar ()) != EOF){
	switch (state){
	case plain:
	    if (c == '\n')
		state = newline;
	    putchar (c);
	    break;
	    
	case newline:
	    if (c == 'F')
		state = seen_f;
	    else {
		state = plain;
		putchar (c);
	    }
	    break;
	    
	case seen_f:
	    if (c == 'r')
		state = seen_r;
	    else {
		printf ("F%c", c);
		state = plain;
	    }
	    break;

	case seen_r:
	    if (c == 'o')
		state = seen_o;
	    else {
		state = plain;
		printf ("Fr%c", c);
	    } 
	    break;

	case seen_o:
	    if (c == 'm'){
		state = seen_m;
	    } else {
		state = plain;
		printf ("Fro%c", c);
	    }
	    break;

	case seen_m:
	    if (c == ' '){
		state = definition;
		printf ("_\bF_\br_\bo_\bm ");
	    } else {
		state = plain;
		printf ("From%c", c);
	    }
	    break;
		
	case header_new:
	    space_seen = 0;
            if (c == ' ' || c == '\t') {
                state = definition;
                putchar (c);
                break;
            }
	    if (c == '\n'){
		state = plain;
		putchar (c);
		break;
	    }
	    
	case header:
	    if (c == '\n'){
		putchar (c);
		state = header_new;
		break;
	    }
	    printf ("_\b%c", c);
	    if (c == ' ')
		space_seen = 1;
	    if (c == ':' && !space_seen)
		state = definition;
	    break;

	case definition:
	    if (c == '\n'){
		putchar (c);
		state = header_new;
		break;
	    }
	    printf ("%c\b%c", c, c);
	    break;
	}
    }
    return (0);
}
