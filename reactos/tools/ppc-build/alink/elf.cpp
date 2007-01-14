#include "alink.h"
#include <assert.h>

#define strupr(x)

unsigned short get16(unsigned char **src) 
{
    unsigned short out = ((*src)[0] & 0xff) | (((*src)[1] & 0xff) << 8);
    *src += 2;
    return out;
}

unsigned long  get32(unsigned char **src) 
{
    unsigned long out =
	 ((*src)[0] & 0xff)        | 
	(((*src)[1] & 0xff) <<  8) |
	(((*src)[2] & 0xff) << 16) |
	(((*src)[3] & 0xff) << 24);
    *src += 4;
    return out;
}

int wantSeg(PSEG seg) 
{
    return 
	!((seg->name.find(".debug") != std::string::npos) ||
	  (seg->name.find(".rela")  != std::string::npos) ||
	  (seg->name.find("symtab") != std::string::npos) ||
	  (seg->name.find("strtab") != std::string::npos));
}

int readElf32Hdr(FILE *objfile, PELF32HDR elf32hdr) 
{
    unsigned char headbuf[52];
    PUCHAR hptr = headbuf;

    if(fread(headbuf,1,52,objfile)!=52)
    {
	return 0;
    }
    
    memcpy(elf32hdr->e_ident, headbuf, EI_NIDENT);
    hptr += EI_NIDENT;
    elf32hdr->e_type =    get16(&hptr);
    elf32hdr->e_machine = get16(&hptr);
    elf32hdr->e_version = get32(&hptr);
    elf32hdr->e_entry =   get32(&hptr);
    elf32hdr->e_phoff =   get32(&hptr);
    elf32hdr->e_shoff =   get32(&hptr);
    elf32hdr->e_flags =   get32(&hptr);
    elf32hdr->e_ehsize =  get16(&hptr);
    elf32hdr->e_phentsize=get16(&hptr);
    elf32hdr->e_phnum =   get16(&hptr);
    elf32hdr->e_shentsize=get16(&hptr);
    elf32hdr->e_shnum =   get16(&hptr);
    elf32hdr->e_shtrndx = get16(&hptr);

    return 1;
}

int readElf32SHdr(FILE *objfile, PELF32SHDR elf32shdr) 
{
    unsigned char buf[40];
    PUCHAR bptr = buf;

    if(fread(buf,1,40,objfile)!=40)
    {
	return 0;
    }
    
    elf32shdr->sh_name =     get32(&bptr);
    elf32shdr->sh_type =     get32(&bptr);
    elf32shdr->sh_flags =    get32(&bptr);
    elf32shdr->sh_addr =     get32(&bptr);
    elf32shdr->sh_offset =   get32(&bptr);
    elf32shdr->sh_size =     get32(&bptr);
    elf32shdr->sh_link =     get32(&bptr);
    elf32shdr->sh_info =     get32(&bptr);
    elf32shdr->sh_addralign =get32(&bptr);
    elf32shdr->sh_entsize =  get32(&bptr);

    return 1;
}

int readElf32Sym(FILE *objfile, PELF32SYM elf32sym)
{
    unsigned char buf[16];
    unsigned char *bptr = buf;

    if(fread(buf,1,16,objfile)!=16)
    {
	return 0;
    }

    elf32sym->st_name = get32(&bptr);
    elf32sym->st_value = get32(&bptr);
    elf32sym->st_size = get32(&bptr);
    elf32sym->st_info = *bptr; bptr++;
    elf32sym->st_other = *bptr; bptr++;
    elf32sym->st_shndx = get16(&bptr);

    return 1;
}

int readElf32Rel(FILE *objfile, PELF32RELX elf32rel)
{
    unsigned char buf[8];
    unsigned char *bptr = buf;

    if(fread(buf,1,8,objfile)!=8)
    {
	return 0;
    }

    elf32rel->r_offset = get32(&bptr);
    elf32rel->r_info   = get32(&bptr);
    elf32rel->r_address= 0;

    return 1;
}

int readElf32Rela(FILE *objfile, PELF32RELX elf32rel)
{
    unsigned char buf[12];
    unsigned char *bptr = buf;

    if(fread(buf,1,12,objfile)!=12)
    {
	return 0;
    }

    elf32rel->r_offset = get32(&bptr);
    elf32rel->r_info   = get32(&bptr);
    elf32rel->r_address= get32(&bptr);

    return 1;
}

void swapscn( PELF32SHDR seca, PELF32SHDR secb ) {
    ELF32SHDR scn;
    memcpy(&scn, seca, sizeof(scn));
    memcpy(seca, secb, sizeof(scn));
    memcpy(secb, &scn, sizeof(scn));
}

void loadelf(FILE *objfile)
{
    unsigned char buf[100];
    PUCHAR bigbuf;
    std::vector<UCHAR> stringList;
    UINT thiscpu;
    UINT symbolPtr = 0;
    UINT numSymbols;
    UINT stringPtr = 0;
    UINT stringSize;
    UINT stringOfs;
    UINT curBase = 0;
    UINT i,j,k,l;
    UINT fileStart;
    UINT relcount = 0;
    UINT bss, bsscur = 0;
    std::vector<COFFSYM> sym;
    PPUBLIC pubdef;
    PCOMDAT comdat;
    PCHAR comdatsym;
    SORTLIST::iterator listnode;
    ELF32HDR elf32hdr;
    std::vector<ELF32SHDR> elf32shdr;
    int elf32sn = 0;
    ELF32SYM elf32sym;
    std::vector<int> stringListOffsets;
    int stringOffset = 0, secNum;
    std::vector<ELF32RELX> rel;

    fileStart=ftell(objfile);

    if(!readElf32Hdr(objfile, &elf32hdr))
    {
        printf("Unable to read from file\n");
	exit(1);
    }

    if( elf32hdr.e_machine == EM_386 )
      cpuIdent = PE_INTEL386;
    else if( elf32hdr.e_machine == EM_PPC )
      cpuIdent = PE_POWERPC;

    if( !cpuIdent ) {
        printf("Unsupported CPU type for module (type %x)\n", elf32hdr.e_machine);
        exit(1);
    }

    elf32shdr.resize((elf32hdr.e_shnum * 2) + 1);
    memset(&elf32shdr[0], 0, (sizeof(elf32shdr[0]) * elf32hdr.e_shnum * 2) + 1);
    stringListOffsets.resize(elf32hdr.e_shnum);

    /* Find a symbol section */
    for( i = 0; i < elf32hdr.e_shnum; i++ ) 
    {
	int seekto = fileStart + elf32hdr.e_shoff + elf32hdr.e_shentsize * i;
	if(fseek(objfile, seekto, 0)==-1) 
	{
	    printf("Short elf file (wanted to seek to %d)\n", seekto);
	    exit(1);
	}

	if(!readElf32SHdr(objfile, &(elf32shdr[i])))
	{
	    printf("Could not read elf section header %d\n", i);
	    exit(1);
	}
    }

    /* Find string tables */
    for( i = 0; i < elf32hdr.e_shnum; i++ ) 
    {
	if( elf32shdr[i].sh_type != SHT_STRTAB ) continue;

	stringPtr = elf32shdr[i].sh_offset;
	stringSize = elf32shdr[i].sh_size + 1;
	
	if(fseek(objfile,fileStart+stringPtr,SEEK_SET)==-1)
	{
	    printf("Short elf file, unable to seek to (%d)\n", stringPtr);
	    exit(1);
	}
	
	if(stringSize)
	{
	    stringList.resize(stringSize + stringOffset);
	    if(fread(&stringList[0]+stringOffset,1,stringSize - 1,objfile)!=stringSize - 1)
	    {
		printf("Invalid elf object file, unable to read string table\n");
		exit(1);
	    }
	    stringList[stringSize-1] = 0;

	    stringListOffsets[i] = stringOffset;
	    stringOffset += stringSize;
	}
    }

    /* Find segment order */
    for( i = 0; i < elf32hdr.e_shnum; i++ )
    {
	const char *segname = (char *)
	    &stringList[0] + stringListOffsets[elf32hdr.e_shtrndx] + 
	    elf32shdr[i].sh_name;

	if(elf32shdr[i].sh_type == SHT_REL || 
	   elf32shdr[i].sh_type == SHT_RELA ||
	   elf32shdr[i].sh_type == SHT_SYMTAB ||
	   elf32shdr[i].sh_type == SHT_STRTAB)
	    continue;

	if(!strcmp(segname,".bss"))
	{
	    bss = i;
	    bsscur = elf32shdr[i].sh_size;
	}
    }

    for( i = 0; i < elf32hdr.e_shnum; i++ )
    {
	if(elf32shdr[i].sh_type == SHT_REL || 
	   elf32shdr[i].sh_type == SHT_RELA ||
	   elf32shdr[i].sh_type == SHT_SYMTAB ||
	   elf32shdr[i].sh_type == SHT_STRTAB)
	    continue;
    }

    memcpy(&elf32shdr[elf32hdr.e_shnum], &elf32shdr[0], sizeof(elf32shdr[0]));

    memcpy(&elf32shdr[0], &elf32shdr[elf32sn], sizeof(elf32shdr[0]) * elf32sn);

    elf32sn = elf32hdr.e_shnum;

    for( i = 0; i < elf32sn; i++ ) 
    {
	if(stringPtr && !symbolPtr && elf32shdr[i].sh_type == SHT_SYMTAB)
	{
	    int actualNum = 0;

	    symbolPtr = elf32shdr[i].sh_offset;
	    numSymbols = elf32shdr[i].sh_size / ELF32SYMSIZE;

	    if(fseek(objfile,fileStart+symbolPtr,SEEK_SET)==-1)
	    {
		printf("Short elf file, unable to seek to (%d)\n", symbolPtr);
		exit(1);
	    }

	    sym.resize(numSymbols);

	    for( j = 0; j < numSymbols; j++ ) 
	    {
		if(!readElf32Sym(objfile, &elf32sym))
		{
		    printf("Could not read elf symbol %d\n", i);
		    exit(1);
		}

		sym[j].name = (char *)
		    (&stringList[stringListOffsets[elf32shdr[i].sh_link]] + 
		     elf32sym.st_name);

#if 0
		const char *segname = 
		    elf32sym.st_shndx & 0x8000 ? 
		    "*SPC*" :
		    (char *)
		    (&stringList[0] + 
		     stringListOffsets[elf32hdr.e_shtrndx] + 
		     elf32shdr[elf32sym.st_shndx].sh_name);

		fprintf(stderr, 
			"S[%05x] value %08x size %08x info %08x "
			"other %08x shndx %08x (%-10s) %s\n",
			j, 
			elf32sym.st_value, elf32sym.st_size, elf32sym.st_info,
			elf32sym.st_other, elf32sym.st_shndx, 
			segname,
			sym[j].name.c_str());
#endif
		
		if(!case_sensitive)
		{
		    strupr(sym[j].name);
		}

		if(elf32sym.st_shndx == SHN_COMMON)
		{
		    UINT thisbss = roundup(bsscur, elf32sym.st_value);
		    /* Round up bss objects */
		    bsscur = thisbss + elf32sym.st_size;
		    sym[j].section = bss;
		    sym[j].value = thisbss;
		    sym[j].coff_class = COFF_SYM_EXTERNAL;
		    //fprintf( stderr, "BSS: @%08x(%04x) %s\n", sym[j].value, elf32sym.st_size, sym[j].name.c_str() );
		}
		else 
		{
		    if(sym[j].name == entryPoint)
			sym[j].coff_class = COFF_SYM_EXTERNAL;
		    else
			sym[j].coff_class = COFF_SYM_STATIC;
		    sym[j].section = elf32sym.st_shndx;
		    sym[j].value = elf32sym.st_value;
		}

		sym[j].extnum = -1;
		sym[j].type = COFFTYPE(DT_FCN,T_NULL);
	    }
	}
	else if(elf32shdr[i].sh_type == SHT_REL || 
		elf32shdr[i].sh_type == SHT_RELA)
	{
	    int rela = elf32shdr[i].sh_type == SHT_RELA;
	    int size = rela ? sizeof(ELF32RELA) : 8;
	    int oldrel = relcount;

	    relcount+=elf32shdr[i].sh_size/size;
	    
	    rel.resize(relcount);
	    fseek(objfile,fileStart+elf32shdr[i].sh_offset,SEEK_SET);
	    
	    for( j = oldrel; j < relcount; j++ ) 
	    {
		if( rela )
		    readElf32Rela(objfile, &rel[j]);
		else
		    readElf32Rel(objfile, &rel[j]);

		rel[j].r_seg = elf32shdr[i].sh_info;
	    }
	}
    }

    for(i=0;i<elf32sn;i++)
    {
	PSEG seg = new SEG();
        seg->name=
	    (char *)&stringList[0] + stringListOffsets[elf32hdr.e_shtrndx] + 
	    elf32shdr[i].sh_name;
        seg->orderindex=0;
        seg->classindex=-1;
        seg->overlayindex=-1;
	if(seg->name == ".bss")
	{
	    seg->length = bsscur;
	}
        else
	{
	    seg->length=elf32shdr[i].sh_size;
	}

	seg->attr=SEG_PUBLIC|SEG_USE32|SEG_DWORD;
        seg->winFlags=WINF_ALIGN_DWORD | WINF_READABLE;
	seg->fileBase = elf32shdr[i].sh_offset;
	seg->setbase(curBase);
	if( wantSeg(seg) ) {
	    curBase += roundup(curBase + seg->length, objectAlign);
	} else {
	    seg->winFlags |= WINF_REMOVE;
	}

	if( elf32shdr[i].sh_flags & SHF_WRITE )
	    seg->winFlags |= WINF_WRITEABLE;

	if( elf32shdr[i].sh_flags == 0 )
	    seg->winFlags |= WINF_COMMENT;
	
	switch( elf32shdr[i].sh_type ) 
	{
	case SHT_PROGBITS:
	    if( elf32shdr[i].sh_flags & SHF_EXECINSTR ) 
		seg->winFlags |= WINF_CODE | WINF_EXECUTE;
	    else 
		seg->winFlags |= WINF_INITDATA;
	    break;

	case SHT_NOBITS:
	    if( elf32shdr[i].sh_flags & SHF_WRITE )
		seg->winFlags |= WINF_UNINITDATA;
	    break;

	default:
	    break;
	}

	if(seg->winFlags & WINF_ALIGN_NOPAD)
	{
	    seg->winFlags &= ~WINF_ALIGN;
	    seg->winFlags |= WINF_ALIGN_BYTE;
	}
	
	/* invert all negative-logic flags */
        seg->winFlags ^= WINF_NEG_FLAGS;
	seg->datmask.resize(roundup(seg->length,8)/8);
	seg->data.resize(roundup(seg->length,8));
	if(elf32shdr[i].sh_type == SHT_NOBITS) 
	{
	    memset(&seg->datmask[0], 0, seg->datmask.size());
	    memset(&seg->data[0], 0, seg->data.size());
	}
	else
	{
	    memset(&seg->datmask[0], 0xff, seg->datmask.size());
	    fseek(objfile, seg->fileBase, 0);
	    fread(&seg->data[0], 1, seg->length, objfile);
	}
	seglist.push_back(seg);
    }

    for( i = 0; i < numSymbols; i++ ) {
	sym[i].segptr = seglist[sym[i].section];

	switch(sym[i].coff_class)
	{
	case COFF_SYM_SECTION: { /* section symbol */
	    /* section symbols declare an extern always, so can use in relocs */
	    /* they may also include a PUBDEF */
	    EXTREC ext;
	    ext.name=(char *)sym[i].name.c_str();
	    ext.pubdef=NULL;
	    ext.modnum=0;
	    ext.flags=EXT_NOMATCH;
	    sym[i].extnum=externs.size();
	    externs.push_back(ext);
	    if(sym[i].section) /* if the section is defined here, make public */
	    {
		pubdef=new PUBLIC();
		pubdef->grpnum=-1;
		pubdef->typenum=0;
		pubdef->modnum=0;
		pubdef->aliasName="";
		pubdef->name = sym[i].name;
		pubdef->ofs=sym[i].value;
		//fprintf(stderr, "sym[%05d] public %s\n", i, sym[i].name.c_str());
		
		if(sym[i].section==-1)
		{
		    pubdef->seg=NULL;
		}
		else
		{
		    pubdef->seg=seglist[sym[i].section];
		}
		if((listnode=publics.find(sym[i].name)) != publics.end())
		{
		    for(j=0;j<listnode->second.size();++j)
		    {
			if(((PPUBLIC)listnode->second[j])->modnum==pubdef->modnum)
			{
			    if(((PPUBLIC)listnode->second[j])->aliasName=="")
			    {
				printf("Duplicate public symbol %s\n",sym[i].name.c_str());
				exit(1);
			    }
			    (*((PPUBLIC)listnode->second[j]))=(*pubdef);
			    pubdef=NULL;
			    break;
			}
		    }
		}
		if(pubdef)
		{
		    SORTLIST::iterator it = publics.find(sym[i].name);
		    if( it == publics.end() ) {
			std::vector<void *> vv;
			vv.push_back((void *)pubdef);
			publics.insert(std::make_pair(sym[i].name, vv));
		    } else {
			it->second.push_back((void *)pubdef);
		    }
		}
	    }
	} break;

	case COFF_SYM_EXTERNAL:
	case COFF_SYM_STATIC: /* allowed, but ignored for now as we only want to process if required */
	case COFF_SYM_LABEL:
	case COFF_SYM_FILE:
	case COFF_SYM_FUNCTION:
	    break;

	default:
	    printf("unsupported symbol class %02X for symbol %s\n",sym[i].coff_class,sym[i].name.c_str());
	    exit(1);
	}
    }

    for(i=0;i<elf32sn;i++)
    {
	for(j=0;j<relcount;j++)
	{
	    if( (rel[j].r_seg != i) || 
		(seglist[rel[j].r_seg]->winFlags & WINF_REMOVE) ) continue;

	    PRELOC reloc = new RELOC();
	    /* get address to relocate */
	    reloc->ofs=rel[j].r_offset;
	    reloc->orig = rel[j];
	    reloc->ftype = REL_SEGFRAME;
	    
	    /* get segment */
	    reloc->seg = seglist[rel[j].r_seg];

	    if( cpuIdent == PE_INTEL386 )
		reloc->disp=4;
	    else
		reloc->disp=rel[j].r_address;

	    /* get relocation target external index */
	    reloc->target=rel[j].r_info >> 8;

	    if(reloc->target>=numSymbols)
	    {
		printf("Invalid ELF object file, undefined symbol\n");
		exit(1);
	    }
	    k=reloc->target;
	    reloc->ttype=REL_EXTDISP; /* assume external reloc */

	    if(sym[k].extnum<0)
	    {
		switch(sym[k].coff_class)
		{
		case COFF_SYM_EXTERNAL: {
		    /* global symbols declare an extern when used in relocs */
		    EXTREC ext;
		    ext.name=sym[k].name;
		    ext.pubdef=NULL;
		    ext.modnum=0;
		    ext.flags=EXT_NOMATCH;
		    sym[k].extnum=externs.size();
		    externs.push_back(ext);
		    /* they may also include a COMDEF or a PUBDEF */
		    /* this is dealt with after all sections loaded, to cater 
		       for COMDAT symbols */
		} break;
		case COFF_SYM_STATIC: /* static symbol */
		case COFF_SYM_LABEL: /* code label symbol */
		    /* update relocation information to reflect symbol */
		    reloc->ttype=REL_SEGDISP;
		    reloc->disp=rel[j].r_address + sym[k].value;
		    reloc->targseg = seglist[sym[k].section];
		    break;
		default:
		    printf("undefined symbol class 0x%02X for symbol %s\n",sym[k].coff_class,sym[k].name.c_str());
		    exit(1);
		}
	    }
	    if(reloc->ttype==REL_EXTONLY ||
	       reloc->ttype==REL_EXTDISP)
	    {
		/* set relocation target to external if sym is external */
		reloc->target=sym[k].extnum;
	    }
	    
	    /* set relocation type */
	    switch (rel[j].r_info & 0xff)
	    {
	    case R_PPC_ADDR32:
		reloc->rtype = FIX_ADDR32;
		break;

	    case R_PPC_UADDR32:
		reloc->rtype = FIX_RVA32;
		break;

	    case R_PPC_REL32:
		reloc->rtype = FIX_REL32;
		break;

	    case R_PPC_ADDR24:
		reloc->rtype = FIX_PPC_ADDR24;
		break;

	    case R_PPC_PLTREL24:
	    case R_PPC_REL24:
		reloc->rtype = FIX_PPC_REL24;
		break;

	    case R_PPC_ADDR16_LO:
		reloc->rtype = FIX_PPC_RVA16LO;
		break;

	    case R_PPC_ADDR16_HA:
		reloc->rtype = FIX_PPC_RVA16HA;
		break;

	    default:
		printf("unsupported ELF relocation type %04X\n",rel[j].r_info & 0xff);
		exit(1);
	    }

	    relocs.push_back(reloc);
        }
    }
    /* build PUBDEFs or COMDEFs for external symbols defined here that aren't COMDAT symbols. */
    for(i=0;i<numSymbols;i++)
    {
	if(sym[i].coff_class!=COFF_SYM_EXTERNAL) continue;
	if(sym[i].isComDat) continue;
	if(sym[i].section<-1)
	{
	    break;
	}

	pubdef=new PUBLIC();
	pubdef->grpnum=-1;
	pubdef->typenum=0;
	pubdef->modnum=0;
	pubdef->name = sym[i].name;
	pubdef->aliasName="";
	pubdef->ofs=sym[i].value;
	
	if(sym[i].section==-1)
	{
	    pubdef->seg=0;
	}
	else
	{
	    pubdef->seg=seglist[sym[i].section];
	}
	if((listnode=publics.find(sym[i].name)) != publics.end())
	{
	    for(j=0;j<listnode->second.size();++j)
	    {
		if(((PPUBLIC)listnode->second[j])->modnum==pubdef->modnum)
		{
		    if(((PPUBLIC)listnode->second[j])->aliasName=="")
		    {
			printf("Duplicate public symbol %s\n",sym[i].name.c_str());
			exit(1);
		    }
		    (*((PPUBLIC)listnode->second[j]))=(*pubdef);
		    pubdef=NULL;
		    break;
		}
	    }
	}
	if(pubdef)
	{
	    SORTLIST::iterator it = publics.find(sym[i].name);
	    if( it == publics.end() ) {
		std::vector<void *> vv;
		vv.push_back((void *)pubdef);
		publics.insert(std::make_pair(sym[i].name,vv));
	    } else {
		it->second.push_back((void *)pubdef);
	    }
	}
    }

    for(i = 0; i < seglist.size(); i++) {
	if(seglist[i]->winFlags & WINF_REMOVE) {
	    seglist.erase(seglist.begin()+i);
	    i--;
	}
    }
}
