#include "alink.h"

#define _strdup strdup

int getBitCount(UINT a)
{
    int count=0;

    while(a)
    {
	if(a&1) count++;
	a>>=1;
    }
    return count;
}

void ReportError(long errnum)
{
    UINT tot,i;
    
    printf("\nError in file at %08lX",filepos);
    switch(errnum)
    {
    case ERR_EXTRA_DATA:
	printf(" - extra data in record\n");
	break;
    case ERR_NO_HEADER:
	printf(" - no header record\n");
	break;
    case ERR_NO_RECDATA:
	printf(" - record data not present\n");
	break;
    case ERR_NO_MEM:
	printf(" - insufficient memory\n");
	break;
    case ERR_INV_DATA:
	printf(" - invalid data address\n");
	break;
    case ERR_INV_SEG:
	printf(" - invalid segment number\n");
	break;
    case ERR_BAD_FIXUP:
	printf(" - invalid fixup record\n");
	break;
    case ERR_BAD_SEGDEF:
	printf(" - invalid segment definition record\n");
	break;
    case ERR_ABS_SEG:
	printf(" - data emitted to absolute segment\n");
	break;
    case ERR_DUP_PUBLIC:
	printf(" - duplicate public definition\n");
	break;
    case ERR_NO_MODEND:
	printf(" - unexpected end of file (no MODEND record)\n");
	break;
    case ERR_EXTRA_HEADER:
	printf(" - duplicate module header\n");
	break;
    case ERR_UNKNOWN_RECTYPE:
	printf(" - unknown object module record type %02X\n",rectype);
	break;
    case ERR_SEG_TOO_LARGE:
	printf(" - 4Gb Non-Absolute segments not supported.\n");
	break;
    case ERR_MULTIPLE_STARTS:
	printf(" - start address defined in more than one module.\n");
	break;
    case ERR_BAD_GRPDEF:
	printf(" - illegal group definition\n");
	break;
    case ERR_OVERWRITE:
	printf(" - overlapping data regions\n");
	break;
    case ERR_INVALID_COMENT:
	printf(" - COMENT record format invalid\n");
	break;
    case ERR_ILLEGAL_IMPORTS:
	printf(" - Imports required to link, and not supported by output file type\n");
	break;
    default:
	printf("\n");
    }
    printf("seg count = %i\n",seglist.size());
    printf("extcount=%i\n",externs.size());
    printf("grpcount=%i\n",grplist.size());
    printf("comcount=%i\n",comdefs.size());
    printf("fixcount=%i\n",relocs.size());
    printf("impcount=%i\n",impdefs.size());
    printf("expcount=%i\n",expdefs.size());
    printf("modcount=%i\n",modname.size());

    for(i=0,tot=0;i<seglist.size();i++)
    {
	if(seglist[i] && seglist[i]->data.size())
	    tot+=seglist[i]->length;
    }
    printf("total segment size=%08X\n",tot);
		
    exit(1);
}

unsigned short wtoupper(unsigned short a)
{
    if(a>=256) return a;
    return toupper(a);
}

int wstricmp(const char *s1,const char *s2)
{
    int i=0;
    unsigned short a,b;

    while(TRUE)
    {
	a=s1[i]+(s1[i+1]<<8);
	b=s2[i]+(s2[i+1]<<8);
	if(wtoupper(a)<wtoupper(b)) return -1;
	if(wtoupper(a)>wtoupper(b)) return +1;
	if(!a) return 0;
	i+=2;
    }
}

int wstrlen(const char *s)
{
    int i;
    for(i=0;s[i]||s[i+1];i+=2);
    return i/2;
}

void *checkMalloc(size_t x)
{
    void *p;

    p=malloc(x);
    if(!p)
    {
	fprintf(stderr,"Error, Insufficient memory in call to malloc\n");
	exit(1);
    }
    return p;
}

void *checkRealloc(void *p,size_t x)
{
    p=realloc(p,x);
    if(!p)
    {
	fprintf(stderr,"Error, Insufficient memory in call to realloc\n");
	exit(1);
    }
    return p;
}

char *checkStrdup(const char *s)
{
    char *p;
    
    if(!s)
    {
	fprintf(stderr,"Error, Taking duplicate of NULL pointer in call to strdup\n");
	exit(1);
    }
    p=strdup(s);
    if(!p)
    {
	fprintf(stderr,"Error, Insufficient memory in call to strdup\n");
	exit(1);
    }
    return p;
}

int _wstricmp(unsigned char *a, unsigned char *b)
{
    uint16_t *a16 = (uint16_t*)a, *b16 = (uint16_t*)b;
    while(*a16 && (*a16 == *b16)) { a16++; b16++; }
    return (*a16 < *b16) ? ((*a16 > *b16) ? 1 : 0) : -1;
}

int _wstrlen(unsigned char *a)
{
    uint16_t *a16 = (uint16_t*)a;
    while(*a16++);
    return a16 - ((uint16_t *)a);
}
