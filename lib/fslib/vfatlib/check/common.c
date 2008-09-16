/* common.c  -  Common functions */

/* Written 1993 by Werner Almesberger */

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */


#include "vfatlib.h"

#define NDEBUG
#include <debug.h>


typedef struct _link {
    void *data;
    struct _link *next;
} LINK;


void die(char *msg,...)
{
    va_list args;

    va_start(args,msg);
    //vfprintf(stderr,msg,args);
    DPRINT1("Unrecoverable problem!\n");
    va_end(args);
    //fprintf(stderr,"\n");
    //exit(1);
}

void pdie(char *msg,...)
{
    va_list args;

    va_start(args,msg);
    //vfprintf(stderr,msg,args);
    DPRINT1("Unrecoverable problem!\n");
    va_end(args);
    //fprintf(stderr,":%s\n",strerror(errno));
    //exit(1);
    DbgBreakPoint();
}


void *vfalloc(int size)
{
    void *ptr;

    ptr = RtlAllocateHeap(RtlGetProcessHeap (),
                           0,
                           size);

    if (ptr == NULL)
    {
        DPRINT1("Allocation failed!\n");
        return NULL;
    }

    return ptr;
}

void vffree(void *ptr)
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, ptr);
}

void *qalloc(void **root,int size)
{
    LINK *link;

    link = vfalloc(sizeof(LINK));
    link->next = *root;
    *root = link;
    return link->data = vfalloc(size);
}


void qfree(void **root)
{
    LINK *this;

    while (*root) {
	this = (LINK *) *root;
	*root = this->next;
	vffree(this->data);
	vffree(this);
    }
}


#ifdef min
#undef min
#endif
int min(int a,int b)
{
    return a < b ? a : b;
}


char get_key(char *valid,char *prompt)
{
#if 0
    int ch,okay;

    while (1) {
	if (prompt) VfatPrint("%s ",prompt);
	fflush(stdout);
	while (ch = getchar(), ch == ' ' || ch == '\t');
	if (ch == EOF) exit(1);
	if (!strchr(valid,okay = ch)) okay = 0;
	while (ch = getchar(), ch != '\n' && ch != EOF);
	if (ch == EOF) exit(1);
	if (okay) return okay;
	VfatPrint("Invalid input.\n");
    }
#else
    return 0;
#endif
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
