/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/ke/module.c
 * PURPOSE:      Loading kernel components
 * PROGRAMMER:   David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *              28/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <coff.h>

#include <ddk/ntddk.h>

#include <internal/iomgr.h>
#include <internal/symbol.h>
#include <internal/string.h>
#include <internal/mm.h>
#include <internal/module.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS **************************************************************/

static unsigned int get_kernel_symbol_addr(char* name)
/*
 * FUNCTION: Get the address of a kernel symbol
 * ARGUMENTS:
 *      name = symbol name
 * RETURNS: The address of the symbol on success
 *          NULL on failure
 */
{
        int i=0;
        while (symbol_table[i].name!=NULL)
        {
                if (strcmp(symbol_table[i].name,name)==0)
                {
                        return(symbol_table[i].value);
                }
                i++;
        }
        return(0);
}

static void get_symbol_name(module* mod, unsigned int i, char* name)
/*
 * FUNCTION: Get the name of a symbol from a loaded module by ordinal
 * ARGUMENTS:
 *      mod = module
 *      i = index of symbol
 *      name (OUT) = pointer to a string where the symbol name will be
 *                   stored
 */
{
        if (mod->sym_list[i].e.e_name[0]!=0)
        {
                strncpy(name,mod->sym_list[i].e.e_name,8);
        }
        else
        {
                strcpy(name,&mod->str_tab[mod->sym_list[i].e.e.e_offset]);
        }
}

static unsigned int get_symbol_value(module* mod, unsigned int i)
/*
 * FUNCTION: Get the value of a module defined symbol
 * ARGUMENTS:
 *      mod = module
 *      i = index of symbol
 * RETURNS: The value of the symbol
 * NOTE: This fixes up references to known sections
 */
{
   char name[255];
   get_symbol_name(mod,i,name);
   //        DPRINT("name %s ",name);
   
   /*
    * Check if the symbol is a section we have relocated
    */
   if (strcmp(name,".text")==0)
     {
	return(mod->text_base);
     }
   if (strcmp(name,".data")==0)
     {
	return(mod->data_base);
     }
   if (strcmp(name,".bss")==0)
     {
	return(mod->bss_base);
     }
   return(mod->sym_list[i].e_value);
}

static int do_reloc32_reloc(module* mod, SCNHDR* scn, RELOC* reloc)
/*
 * FUNCTION: Performs a reloc32 relocation on a loaded module
 * ARGUMENTS:
 *         mod = module to perform the relocation on
 *         scn = Section to perform the relocation in
 *         reloc = Pointer to a data structure describing the relocation
 * RETURNS: Success or failure
 * NOTE: This fixes up an undefined reference to a kernel function in a module
 */
{
   char name[255];
   unsigned int val;
   unsigned int * loc;
   
   memset(name,0,255);
   get_symbol_name(mod,reloc->r_symndx,name);
   val = get_kernel_symbol_addr(name);
   if (val==0)
     {
	DbgPrint("Undefined symbol %s in module\n",name);
	return(0);
     }
   //        DPRINT("REL32 value %x name %s\n",val,name);
   //        printk("value %x\n",val);
   loc=(unsigned int *)(mod->base+reloc->r_vaddr);
   //        printk("old %x ",*loc);
   (*loc) = (*loc) + val - mod->base + scn->s_vaddr;
   
   //        printk("new %x\n",*loc);
   
   return(1);
}

static int do_addr32_reloc(module* mod, SCNHDR* scn, RELOC* reloc)
/*
 * FUNCTION: Performs a addr32 relocation on a loaded module
 * ARGUMENTS:
 *         mod = module to perform the relocation on
 *         scn = Section to perform the relocation in
 *         reloc = Pointer to a data structure describing the relocation
 * RETURNS: Success or failure
 * NOTE: This fixes up a relocation needed when changing the base address of a
 * module
 */
{
   unsigned int value;
   unsigned int * loc;
   //        printk("ADDR32 ");
   
   
   value=get_symbol_value(mod,reloc->r_symndx);
   
   //        printk("value %x\n",value);
   
   loc=(unsigned int *)(mod->base+reloc->r_vaddr);
//   DPRINT("ADDR32 loc %x value %x *loc %x ",loc,value,*loc);
   *loc=(*loc)+mod->base;
   //        *loc = value;
   //      *loc=(*loc)+value
   
//   DPRINT("*loc %x\n",*loc);
   
   return(1);
}

static BOOLEAN do_reloc(module* mod, unsigned int scn_idx)
/*
 * FUNCTION: Do the relocations for a module section
 * ARGUMENTS:
 *          mod = Pointer to the module 
 *          scn_idx = Index of the section to be relocated
 * RETURNS: Success or failure
 */
{
   SCNHDR* scn = &mod->scn_list[scn_idx];
   RELOC* reloc = (RELOC *)(mod->raw_data_off + scn->s_relptr);
   int j;
   
   DPRINT("scn_idx %d name %.8s relocs %d\n",scn_idx,
	  mod->scn_list[scn_idx].s_name,scn->s_nreloc);
   
   for (j=0;j<scn->s_nreloc;j++)
     {
	//                printk("vaddr %x ",reloc->r_vaddr);
	//                printk("symndex %x ",reloc->r_symndx);

	switch(reloc->r_type)
	  {
	   case RELOC_ADDR32:
	     if (!do_addr32_reloc(mod,scn,reloc))
	       {
		  return(0);
	       }
	     break;
	     
	   case RELOC_REL32:
	     if (!do_reloc32_reloc(mod,scn,reloc))
	       {
		  return(0);
	       }
	     break;

	   default:
	     DbgPrint("Unknown relocation type %x at %d in module\n",
		    reloc->r_type,j);
	     return(0);
	  }
	
	reloc = (RELOC *)(((unsigned int)reloc) + sizeof(RELOC));
     }
   
   DPRINT("Done relocations for %.8s\n",mod->scn_list[scn_idx].s_name);
   
   return(1);
}                   

BOOLEAN process_boot_module(unsigned int start)
/*
 * FUNCTION: Processes and initializes a module whose disk image has been 
 * loaded
 * ARGUMENTS:
 *         start = start of the module in memory
 * RETURNS: Success or failure
 */
{
   FILHDR hdr;
   AOUTHDR ohdr;
   module* mod;
   unsigned int entry=0;
   unsigned int found_entry=0;
   PDRIVER_INITIALIZE func;
   int i;
   
   DPRINT("process_boot_module(start %x)\n",start);
   DPRINT("n = %x\n",*((unsigned int *)start));
   mod=(module *)ExAllocatePool(NonPagedPool,sizeof(module));
   
   DPRINT("magic %x\n",((FILHDR *)start)->f_magic);

   memcpy(&hdr,(void *)start,FILHSZ);

   if (I386BADMAG(hdr))
     {
        DbgPrint("(%s:%d) Module has bad magic value (%x)\n",__FILE__,
               __LINE__,hdr.f_magic);
	return(0);
     }
   
   memcpy(&ohdr,(void *)(start+FILHSZ),AOUTSZ);
   
   mod->sym_list = (SYMENT *)(start + hdr.f_symptr);
   
   mod->str_tab = (char *)(start + hdr.f_symptr +
			   hdr.f_nsyms * SYMESZ);
   
   mod->scn_list = (SCNHDR *)(start+FILHSZ+hdr.f_opthdr);
   mod->size=0;
   mod->raw_data_off = start;
   
   /*
    * Determine the length of the module 
    */
   for (i=0;i<hdr.f_nscns;i++)
     {
	DPRINT("Section name: %.8s\n",mod->scn_list[i].s_name);
	DPRINT("size %x vaddr %x size %x\n",mod->size,
	       mod->scn_list[i].s_vaddr,mod->scn_list[i].s_size);
	if (mod->scn_list[i].s_flags & STYP_TEXT)
	  {
	     mod->text_base=mod->scn_list[i].s_vaddr;
	  }
	if (mod->scn_list[i].s_flags & STYP_DATA)
	  {
	     mod->data_base=mod->scn_list[i].s_vaddr;
	  }
	if (mod->scn_list[i].s_flags & STYP_BSS)
	  {
	     mod->bss_base=mod->scn_list[i].s_vaddr;
	  }
	if (mod->size <
	    (mod->scn_list[i].s_vaddr + mod->scn_list[i].s_size))
	  {
	     mod->size = mod->size + mod->scn_list[i].s_vaddr +
	       mod->scn_list[i].s_size;
	  }
        }
   
   CHECKPOINT;
   mod->base = (unsigned int)MmAllocateSection(mod->size);
   if (mod->base == 0)
     {
	DbgPrint("Failed to alloc section for module\n");
	return(0);
     }
   CHECKPOINT;
   
   /*
    * Adjust section vaddrs for allocated area
    */
   mod->data_base=mod->data_base+mod->base;
   mod->text_base=mod->text_base+mod->base;
   mod->bss_base=mod->bss_base+mod->base;
   
   /*
    * Relocate module and fixup imports
    */
   for (i=0;i<hdr.f_nscns;i++)
     {
	if (mod->scn_list[i].s_flags & STYP_TEXT ||
	    mod->scn_list[i].s_flags & STYP_DATA)
	  {
	     memcpy((void *)(mod->base + mod->scn_list[i].s_vaddr),
		    (void *)(start + mod->scn_list[i].s_scnptr),
		    mod->scn_list[i].s_size);
	     if (!do_reloc(mod,i))
	       {
		  DPRINT("Relocation failed for section %s\n",
			 mod->scn_list[i].s_name);
		  return(0);
	       }
	  }
	if (mod->scn_list[i].s_flags & STYP_BSS)
	  {
             memset((void *)(mod->base + mod->scn_list[i].s_vaddr),0,
                    mod->scn_list[i].s_size);
          }
     }
   
   DPRINT("Allocate base: %x\n",mod->base);
   
   /*
    * Find the entry point
    */
   
   for (i=0;i<hdr.f_nsyms;i++)
     {
	char name[255];
	get_symbol_name(mod,i,name);       	
	if (strcmp(name,"_DriverEntry")==0)
	  {
	     entry = mod->sym_list[i].e_value;
	     found_entry=1;
	     DPRINT("Found entry at %x\n",entry);
	  }
     }

   if (!found_entry)
        {
	   DbgPrint("No module entry point defined\n");
	   return(0);
        }
   
   /*
    * Call the module initalization routine
    */
   func = (PDRIVER_INITIALIZE)(mod->base + entry);
   return(InitalizeLoadedDriver(func));
}
