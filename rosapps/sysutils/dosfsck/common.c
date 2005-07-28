/* common.c  -  Common functions */

/* Written 1993 by Werner Almesberger */

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "common.h"


typedef struct _link {
    void *data;
    struct _link *next;
} LINK;


void die(char *msg,...)
{
    va_list args;

    va_start(args,msg);
    vfprintf(stderr,msg,args);
    va_end(args);
    fprintf(stderr,"\n");
    exit(1);
}


void pdie(char *msg,...)
{
    va_list args;

    va_start(args,msg);
    vfprintf(stderr,msg,args);
    va_end(args);
    fprintf(stderr,":%s\n",strerror(errno));
    exit(1);
}


void *alloc(int size)
{
    void *this;

    if ((this = malloc(size))) return this;
    pdie("malloc");
    return NULL; /* for GCC */
}


void *qalloc(void **root,int size)
{
    LINK *link;

    link = alloc(sizeof(LINK));
    link->next = *root;
    *root = link;
    return link->data = alloc(size);
}


void qfree(void **root)
{
    LINK *this;

    while (*root) {
	this = (LINK *) *root;
	*root = this->next;
	free(this->data);
	free(this);
    }
}


int min(int a,int b)
{
    return a < b ? a : b;
}


char get_key(char *valid,char *prompt)
{
    int ch,okay;

    while (1) {
	if (prompt) printf("%s ",prompt);
	fflush(stdout);
	while (ch = getchar(), ch == ' ' || ch == '\t');
	if (ch == EOF) exit(1);
	if (!strchr(valid,okay = ch)) okay = 0;
	while (ch = getchar(), ch != '\n' && ch != EOF);
	if (ch == EOF) exit(1);
	if (okay) return okay;
	printf("Invalid input.\n");
    }
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
