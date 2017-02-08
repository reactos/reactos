/*
 * Read VC++ debug information from COFF and eventually
 * from PDB files.
 *
 * Copyright (C) 1996,      Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004,      Eric Pouech.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Note - this handles reading debug information for 32 bit applications
 * that run under Windows-NT for example.  I doubt that this would work well
 * for 16 bit applications, but I don't think it really matters since the
 * file format is different, and we should never get in here in such cases.
 *
 * TODO:
 *	Get 16 bit CV stuff working.
 *	Add symbol size to internal symbol table.
 */

#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp_coff);

/*========================================================================
 * Process COFF debug information.
 */

struct CoffFile
{
    unsigned int                startaddr;
    unsigned int                endaddr;
    struct symt_compiland*      compiland;
    int                         linetab_offset;
    int                         linecnt;
    struct symt**               entries;
    int		                neps;
    int		                neps_alloc;
};

struct CoffFileSet
{
    struct CoffFile*    files;
    int		        nfiles;
    int		        nfiles_alloc;
};

static const char*	coff_get_name(const IMAGE_SYMBOL* coff_sym, 
                                      const char* coff_strtab)
{
    static	char	namebuff[9];
    const char*		nampnt;

    if (coff_sym->N.Name.Short)
    {
        memcpy(namebuff, coff_sym->N.ShortName, 8);
        namebuff[8] = '\0';
        nampnt = &namebuff[0];
    }
    else
    {
        nampnt = coff_strtab + coff_sym->N.Name.Long;
    }

    if (nampnt[0] == '_') nampnt++;
    return nampnt;
}

static int coff_add_file(struct CoffFileSet* coff_files, struct module* module,
                         const char* filename)
{
    struct CoffFile* file;

    if (coff_files->nfiles + 1 >= coff_files->nfiles_alloc)
    {
        if (coff_files->files)
        {
            coff_files->nfiles_alloc *= 2;
            coff_files->files = HeapReAlloc(GetProcessHeap(), 0, coff_files->files,
                                            coff_files->nfiles_alloc * sizeof(struct CoffFile));
        }
        else
        {
            coff_files->nfiles_alloc = 16;
            coff_files->files = HeapAlloc(GetProcessHeap(), 0,
                                          coff_files->nfiles_alloc * sizeof(struct CoffFile));
        }
    }
    file = coff_files->files + coff_files->nfiles;
    file->startaddr = 0xffffffff;
    file->endaddr   = 0;
    file->compiland = symt_new_compiland(module, 0,
                                         source_new(module, NULL, filename));
    file->linetab_offset = -1;
    file->linecnt = 0;
    file->entries = NULL;
    file->neps = file->neps_alloc = 0;

    return coff_files->nfiles++;
}

static void coff_add_symbol(struct CoffFile* coff_file, struct symt* sym)
{
    if (coff_file->neps + 1 >= coff_file->neps_alloc)
    {
        if (coff_file->entries)
        {
            coff_file->neps_alloc *= 2;
            coff_file->entries = HeapReAlloc(GetProcessHeap(), 0, coff_file->entries,
                                             coff_file->neps_alloc * sizeof(struct symt*));
        }
        else
        {
            coff_file->neps_alloc = 32;
            coff_file->entries = HeapAlloc(GetProcessHeap(), 0,
                                           coff_file->neps_alloc * sizeof(struct symt*));
        }
    }
    coff_file->entries[coff_file->neps++] = sym;
}

DECLSPEC_HIDDEN BOOL coff_process_info(const struct msc_debug_info* msc_dbg)
{
    const IMAGE_AUX_SYMBOL*		aux;
    const IMAGE_COFF_SYMBOLS_HEADER*	coff;
    const IMAGE_LINENUMBER*		coff_linetab;
    const IMAGE_LINENUMBER*		linepnt;
    const char*                         coff_strtab;
    const IMAGE_SYMBOL* 		coff_sym;
    const IMAGE_SYMBOL* 		coff_symbols;
    struct CoffFileSet	                coff_files;
    int				        curr_file_idx = -1;
    unsigned int		        i;
    int			       	        j;
    int			       	        k;
    int			       	        l;
    int		       		        linetab_indx;
    const char*                         nampnt;
    int		       		        naux;
    BOOL                                ret = FALSE;
    ULONG64                             addr;

    TRACE("Processing COFF symbols...\n");

    assert(sizeof(IMAGE_SYMBOL) == IMAGE_SIZEOF_SYMBOL);
    assert(sizeof(IMAGE_LINENUMBER) == IMAGE_SIZEOF_LINENUMBER);

    coff_files.files = NULL;
    coff_files.nfiles = coff_files.nfiles_alloc = 0;

    coff = (const IMAGE_COFF_SYMBOLS_HEADER*)msc_dbg->root;

    coff_symbols = (const IMAGE_SYMBOL*)((const char *)coff + coff->LvaToFirstSymbol);
    coff_linetab = (const IMAGE_LINENUMBER*)((const char *)coff + coff->LvaToFirstLinenumber);
    coff_strtab = (const char*)(coff_symbols + coff->NumberOfSymbols);

    linetab_indx = 0;

    for (i = 0; i < coff->NumberOfSymbols; i++)
    {
        coff_sym = coff_symbols + i;
        naux = coff_sym->NumberOfAuxSymbols;

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_FILE)
	{
            curr_file_idx = coff_add_file(&coff_files, msc_dbg->module, 
                                          (const char*)(coff_sym + 1));
            TRACE("New file %s\n", (const char*)(coff_sym + 1));
            i += naux;
            continue;
	}

        if (curr_file_idx < 0)
        {
            assert(coff_files.nfiles == 0 && coff_files.nfiles_alloc == 0);
            curr_file_idx = coff_add_file(&coff_files, msc_dbg->module, "<none>");
            TRACE("New file <none>\n");
        }

        /*
         * This guy marks the size and location of the text section
         * for the current file.  We need to keep track of this so
         * we can figure out what file the different global functions
         * go with.
         */
        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC &&
            naux != 0 && coff_sym->Type == 0 && coff_sym->SectionNumber == 1)
	{
            aux = (const IMAGE_AUX_SYMBOL*) (coff_sym + 1);

            if (coff_files.files[curr_file_idx].linetab_offset != -1)
	    {
                /*
                 * Save this so we can still get the old name.
                 */
                const char* fn;

                fn = source_get(msc_dbg->module,
                                coff_files.files[curr_file_idx].compiland->source);

                TRACE("Duplicating sect from %s: %x %x %x %d %d\n",
                      fn, aux->Section.Length,
                      aux->Section.NumberOfRelocations,
                      aux->Section.NumberOfLinenumbers,
                      aux->Section.Number, aux->Section.Selection);
                TRACE("More sect %d %s %08x %d %d %d\n",
                      coff_sym->SectionNumber,
                      coff_get_name(coff_sym, coff_strtab),
                      coff_sym->Value, coff_sym->Type,
                      coff_sym->StorageClass, coff_sym->NumberOfAuxSymbols);

                /*
                 * Duplicate the file entry.  We have no way to describe
                 * multiple text sections in our current way of handling things.
                 */
                coff_add_file(&coff_files, msc_dbg->module, fn);
	    }
            else
	    {
                TRACE("New text sect from %s: %x %x %x %d %d\n",
                      source_get(msc_dbg->module, coff_files.files[curr_file_idx].compiland->source),
                      aux->Section.Length,
                      aux->Section.NumberOfRelocations,
                      aux->Section.NumberOfLinenumbers,
                      aux->Section.Number, aux->Section.Selection);
	    }

            if (coff_files.files[curr_file_idx].startaddr > coff_sym->Value)
	    {
                coff_files.files[curr_file_idx].startaddr = coff_sym->Value;
	    }

            if (coff_files.files[curr_file_idx].endaddr < coff_sym->Value + aux->Section.Length)
	    {
                coff_files.files[curr_file_idx].endaddr = coff_sym->Value + aux->Section.Length;
	    }

            coff_files.files[curr_file_idx].linetab_offset = linetab_indx;
            coff_files.files[curr_file_idx].linecnt = aux->Section.NumberOfLinenumbers;
            linetab_indx += aux->Section.NumberOfLinenumbers;
            i += naux;
            continue;
	}

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC && naux == 0 && 
            coff_sym->SectionNumber == 1)
	{
            DWORD base = msc_dbg->sectp[coff_sym->SectionNumber - 1].VirtualAddress;
            /*
             * This is a normal static function when naux == 0.
             * Just register it.  The current file is the correct
             * one in this instance.
             */
            nampnt = coff_get_name(coff_sym, coff_strtab);

            TRACE("\tAdding static symbol %s\n", nampnt);

            /* FIXME: was adding symbol to this_file ??? */
            coff_add_symbol(&coff_files.files[curr_file_idx],
                            &symt_new_function(msc_dbg->module, 
                                               coff_files.files[curr_file_idx].compiland, 
                                               nampnt,
                                               msc_dbg->module->module.BaseOfImage + base + coff_sym->Value,
                                               0 /* FIXME */,
                                               NULL /* FIXME */)->symt);
            continue;
	}

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            ISFCN(coff_sym->Type) && coff_sym->SectionNumber > 0)
	{
            struct symt_compiland* compiland = NULL;
            DWORD base = msc_dbg->sectp[coff_sym->SectionNumber - 1].VirtualAddress;
            nampnt = coff_get_name(coff_sym, coff_strtab);

            TRACE("%d: %s %s\n",
                  i, wine_dbgstr_longlong(msc_dbg->module->module.BaseOfImage + base + coff_sym->Value),
                  nampnt);
            TRACE("\tAdding global symbol %s (sect=%s)\n",
                  nampnt, msc_dbg->sectp[coff_sym->SectionNumber - 1].Name);

            /*
             * Now we need to figure out which file this guy belongs to.
             */
            for (j = 0; j < coff_files.nfiles; j++)
	    {
                if (coff_files.files[j].startaddr <= base + coff_sym->Value
                     && coff_files.files[j].endaddr > base + coff_sym->Value)
		{
                    compiland = coff_files.files[j].compiland;
                    break;
		}
	    }
            if (j < coff_files.nfiles)
            {
                coff_add_symbol(&coff_files.files[j],
                                &symt_new_function(msc_dbg->module, compiland, nampnt, 
                                                   msc_dbg->module->module.BaseOfImage + base + coff_sym->Value,
                                                   0 /* FIXME */, NULL /* FIXME */)->symt);
            } 
            else 
            {
                symt_new_function(msc_dbg->module, NULL, nampnt, 
                                  msc_dbg->module->module.BaseOfImage + base + coff_sym->Value,
                                  0 /* FIXME */, NULL /* FIXME */);
            }
            i += naux;
            continue;
	}

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            coff_sym->SectionNumber > 0)
	{
            DWORD base = msc_dbg->sectp[coff_sym->SectionNumber - 1].VirtualAddress;
            struct location loc;

            /*
             * Similar to above, but for the case of data symbols.
             * These aren't treated as entrypoints.
             */
            nampnt = coff_get_name(coff_sym, coff_strtab);

            TRACE("%d: %s %s\n",
                  i, wine_dbgstr_longlong(msc_dbg->module->module.BaseOfImage + base + coff_sym->Value),
                  nampnt);
            TRACE("\tAdding global data symbol %s\n", nampnt);

            /*
             * Now we need to figure out which file this guy belongs to.
             */
            loc.kind = loc_absolute;
            loc.reg = 0;
            loc.offset = msc_dbg->module->module.BaseOfImage + base + coff_sym->Value;
            symt_new_global_variable(msc_dbg->module, NULL, nampnt, TRUE /* FIXME */,
                                     loc, 0 /* FIXME */, NULL /* FIXME */);
            i += naux;
            continue;
	}

        if (coff_sym->StorageClass == IMAGE_SYM_CLASS_STATIC && naux == 0)
	{
            /*
             * Ignore these.  They don't have anything to do with
             * reality.
             */
            continue;
	}

        TRACE("Skipping unknown entry '%s' %d %d %d\n",
              coff_get_name(coff_sym, coff_strtab),
              coff_sym->StorageClass, coff_sym->SectionNumber, naux);
        
        /*
         * For now, skip past the aux entries.
         */
        i += naux;
    }

    if (coff_files.files != NULL)
    {
        /*
         * OK, we now should have a list of files, and we should have a list
         * of entrypoints.  We need to sort the entrypoints so that we are
         * able to tie the line numbers with the given functions within the
         * file.
         */
        for (j = 0; j < coff_files.nfiles; j++)
        {
            if (coff_files.files[j].entries != NULL)
            {
                qsort(coff_files.files[j].entries, coff_files.files[j].neps,
                      sizeof(struct symt*), symt_cmp_addr);
            }
        }

        /*
         * Now pick apart the line number tables, and attach the entries
         * to the given functions.
         */
        for (j = 0; j < coff_files.nfiles; j++)
        {
            l = 0;
            if (coff_files.files[j].neps != 0)
            {
                for (k = 0; k < coff_files.files[j].linecnt; k++)
                {
                    linepnt = coff_linetab + coff_files.files[j].linetab_offset + k;
                    /*
                     * If we have spilled onto the next entrypoint, then
                     * bump the counter..
                     */
                    for (; l+1 < coff_files.files[j].neps; l++)
                    {
                        if (symt_get_address(coff_files.files[j].entries[l+1], &addr) &&
                            msc_dbg->module->module.BaseOfImage + linepnt->Type.VirtualAddress < addr)
                        {
                            if (coff_files.files[j].entries[l+1]->tag == SymTagFunction)
                            {
                                /*
                                 * Add the line number.  This is always relative to the
                                 * start of the function, so we need to subtract that offset
                                 * first.
                                 */
                                symt_add_func_line(msc_dbg->module,
                                                   (struct symt_function*)coff_files.files[j].entries[l+1],
                                                   coff_files.files[j].compiland->source,
                                                   linepnt->Linenumber,
                                                   msc_dbg->module->module.BaseOfImage + linepnt->Type.VirtualAddress - addr);
                            }
                            break;
                        }
                    }
                }
            }
        }

        for (j = 0; j < coff_files.nfiles; j++)
	{
            HeapFree(GetProcessHeap(), 0, coff_files.files[j].entries);
	}
        HeapFree(GetProcessHeap(), 0, coff_files.files);
        msc_dbg->module->module.SymType = SymCoff;
        /* FIXME: we could have a finer grain here */
        msc_dbg->module->module.LineNumbers = TRUE;
        msc_dbg->module->module.GlobalSymbols = TRUE;
        msc_dbg->module->module.TypeInfo = FALSE;
        msc_dbg->module->module.SourceIndexed = TRUE;
        msc_dbg->module->module.Publics = TRUE;
        ret = TRUE;
    }

    return ret;
}
