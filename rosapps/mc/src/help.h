#ifndef __HELP_H
#define __HELP_H

/* This file is included by help.c and man2hlp.c */

/* Some useful constants */
#define CHAR_NODE_END		'\04'
#define CHAR_LINK_START		'\01'
#define CHAR_LINK_POINTER	'\02'
#define CHAR_LINK_END		'\03'
#define CHAR_ALTERNATE		'\05'
#define CHAR_NORMAL		'\06'
#define CHAR_VERSION		'\07'
#define CHAR_BOLD_ON		'\010'
#define CHAR_BOLD_OFF		'\013'
#define CHAR_MCLOGO		'\014'
#define CHAR_TEXTONLY_START	'\016'
#define CHAR_TEXTONLY_END	'\017'
#define CHAR_XONLY_START	'\020'
#define CHAR_XONLY_END		'\021'
#define CHAR_TITLE_ON		'\022'
#define CHAR_TITLE_OFF		'\023'
#define CHAR_ITALIC_ON		'\024'
#define CHAR_RESERVED		'\025'
#define STRING_LINK_START	"\01"
#define STRING_LINK_POINTER	"\02"
#define STRING_LINK_END		"\03"
#define STRING_NODE_END		"\04"

void interactive_display (char *filename, char *node);
char *help_follow_link (char *start, char *selected_item);
#endif	/* __HELP_H */
