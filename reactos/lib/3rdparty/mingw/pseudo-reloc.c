/* Only necesary on x86 and amd64 targets */
#if defined(__i386__) || defined(__x86_64__)
#include <windows.h>
#include <stdio.h>

extern char __RUNTIME_PSEUDO_RELOC_LIST__;
extern char __RUNTIME_PSEUDO_RELOC_LIST_END__;
extern char _image_base__;

typedef struct {
  DWORD addend;
  DWORD target;
} runtime_pseudo_reloc_item_v1;

typedef struct {
  DWORD sym;
  DWORD target;
  DWORD flags;
} runtime_pseudo_reloc_item_v2;

typedef struct {
	DWORD magic1;
	DWORD magic2;
	DWORD version;
} runtime_pseudo_reloc_v2;

#define RP_VERSION_V1 0
#define RP_VERSION_V2 1

static void
do_pseudo_reloc (void* start,void *end,void *base)
{
  ptrdiff_t addr_imp, reldata;
  ptrdiff_t reloc_target = (ptrdiff_t) ((char *)end - (char*)start);
  runtime_pseudo_reloc_v2 *v2_hdr = (runtime_pseudo_reloc_v2 *) start;
  runtime_pseudo_reloc_item_v2 *r;

  if (reloc_target < 8)
    return;
  /* Check if this is old version pseudo relocation version.  */
  if (reloc_target >= 12
      && v2_hdr->magic1 == 0 && v2_hdr->magic2 == 0
      && v2_hdr->version == RP_VERSION_V1)
      v2_hdr++;
  if (v2_hdr->magic1 != 0 || v2_hdr->magic2 != 0)
    {
      runtime_pseudo_reloc_item_v1 *o;
      for (o = (runtime_pseudo_reloc_item_v1 *) v2_hdr; o < (runtime_pseudo_reloc_item_v1 *)end; o++)
        {
	  reloc_target = (ptrdiff_t) base + o->target;
	  *((DWORD*) reloc_target) += o->addend;
        }
      return;
    }
  /* Check if this is a known version.  */
  if (v2_hdr->version != RP_VERSION_V2)
    {
      fprintf (stderr, "pseudo_relocation protocol version %d is unknown to this runtime.\n",
	       (int) v2_hdr->version);
      return;
    }
  /* Walk over header.  */
  r = (runtime_pseudo_reloc_item_v2 *) &v2_hdr[1];

  for (; r < (runtime_pseudo_reloc_item_v2 *) end; r++)
    {
      reloc_target = (ptrdiff_t) base + r->target;
      addr_imp = (ptrdiff_t) base + r->sym;
      addr_imp = *((ptrdiff_t *) addr_imp);

      switch ((r->flags&0xff))
        {
          case 8:
	    reldata = (ptrdiff_t) (*((unsigned char *)reloc_target));
	    if ((reldata&0x80) != 0)
	      reldata |= ~((ptrdiff_t) 0xff);
	    break;
	  case 16:
	    reldata = (ptrdiff_t) (*((unsigned short *)reloc_target));
	    if ((reldata&0x8000) != 0)
	      reldata |= ~((ptrdiff_t) 0xffff);
	    break;
	  case 32:
	    reldata = (ptrdiff_t) (*((unsigned int *)reloc_target));
#ifdef _WIN64
	    if ((reldata&0x80000000) != 0)
	      reldata |= ~((ptrdiff_t) 0xffffffff);
#endif
	    break;
#ifdef _WIN64
	  case 64:
	    reldata = (ptrdiff_t) (*((unsigned long long *)reloc_target));
	    break;
#endif
	  default:
	    reldata=0;
	    fprintf(stderr, "Unknown pseudo relocation bit size %d\n",(int) (r->flags & 0xff));
	    break;
        }
      reldata -= ((ptrdiff_t) base + r->sym);
      reldata += addr_imp;
      switch ((r->flags & 0xff))
        {
         case 8:
	   *((unsigned char*)reloc_target)=(unsigned char) reldata;
	   break;
	 case 16:
	   *((unsigned short*)reloc_target)=(unsigned short) reldata;
	   break;
	 case 32:
	   *((unsigned int*)reloc_target)=(unsigned int) reldata;
	   break;
#ifdef _WIN64
	 case 64:
	   *((unsigned long long*)reloc_target)=(unsigned long long) reldata;
	   break;
#endif
        }
    }
}

void
_pei386_runtime_relocator ()
{
  do_pseudo_reloc (&__RUNTIME_PSEUDO_RELOC_LIST__,&__RUNTIME_PSEUDO_RELOC_LIST_END__,&_image_base__);
}
#endif
