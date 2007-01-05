#include "alink.h"

#define strupr(x)

void loadcoff(FILE *objfile)
{
    unsigned char headbuf[20];
    unsigned char buf[100];
    PUCHAR bigbuf;
    std::vector<UCHAR> stringList;
    UINT thiscpu;
    UINT numSect;
    UINT headerSize;
    UINT symbolPtr;
    UINT numSymbols;
    UINT stringPtr;
    UINT stringSize;
    UINT stringOfs;
    UINT i,j,k,l;
    UINT fileStart;
    UINT minseg;
    UINT numrel;
    UINT relofs;
    UINT relshift;
    UINT sectname;
    long sectorder;
    std::vector<COFFSYM> sym;
    UINT combineType;
    PPUBLIC pubdef;
    PCOMDAT comdat;
    PCHAR comdatsym;
    std::vector<std::string> nlist;
    SORTLIST::iterator listnode;

    minseg=seglist.size();
    fileStart=ftell(objfile);

    if(fread(headbuf,1,20,objfile)!=20)
    {
        printf("Unable to read from file\n");
        exit(1);
    }
    thiscpu=headbuf[0]+256*headbuf[1];
    if(!thiscpu)
    {
	/* if we've got an import module, start at the beginning */
	fseek(objfile,fileStart,SEEK_SET);
	/* and load it */
	loadCoffImport(objfile);
	return;
    }
    
    if(thiscpu==0x14c)
      cpuIdent = PE_INTEL386;
    if(thiscpu==0x1f0)
      cpuIdent = PE_POWERPC;

    if(!cpuIdent)
    {
        printf("Unsupported CPU type for module (type %x)\n", thiscpu);
        exit(1);
    }

    numSect=get16(headbuf, PE_NUMOBJECTS-PE_MACHINEID);
    symbolPtr=get32(headbuf, PE_SYMBOLPTR-PE_MACHINEID);
    numSymbols=get32(headbuf, PE_NUMSYMBOLS-PE_MACHINEID);

    if(headbuf[PE_HDRSIZE-PE_MACHINEID]|headbuf[PE_HDRSIZE-PE_MACHINEID+1])
    {
        printf("warning, optional header discarded\n");
	headerSize=headbuf[PE_HDRSIZE-PE_MACHINEID]+256*headbuf[PE_HDRSIZE-PE_MACHINEID+1];
    }
    else
	headerSize=0;
    headerSize+=PE_BASE_HEADER_SIZE-PE_MACHINEID;
    
    stringPtr=symbolPtr+numSymbols*PE_SYMBOL_SIZE;
    if(stringPtr)
    {
        fseek(objfile,fileStart+stringPtr,SEEK_SET);
        if(fread(buf,1,4,objfile)!=4)
        {
            printf("Invalid COFF object file, unable to read string table size\n");
            exit(1);
        }
        stringSize=buf[0]+(buf[1]<<8)+(buf[2]<<16)+(buf[3]<<24);
        if(!stringSize) stringSize=4;
        if(stringSize<4)
        {
            printf("Invalid COFF object file, bad string table size %i\n",stringSize);
            exit(1);
        }
        stringPtr+=4;
        stringSize-=4;
    }
    else
    {
        stringSize=0;
    }
    if(stringSize)
    {
        stringList.resize(stringSize);
        if(fread(&stringList[0],1,stringSize,objfile)!=stringSize)
        {
            printf("Invalid COFF object file, unable to read string table\n");
            exit(1);
        }
        if(stringList[stringSize-1])
        {
            printf("Invalid COFF object file, last string unterminated\n");
            exit(1);
        }
    }
    else
    {
	stringList.resize(0);
    }
    
    if(symbolPtr && numSymbols)
    {
        fseek(objfile,fileStart+symbolPtr,SEEK_SET);
        sym.resize(numSymbols);
        for(i=0;i<numSymbols;i++)
        {
            if(fread(buf,1,PE_SYMBOL_SIZE,objfile)!=PE_SYMBOL_SIZE)
            {
                printf("Invalid COFF object file, unable to read symbols\n");
                exit(1);
            }
            if(get32(buf,0))
            {
                sym[i].name=std::string((char *)buf,8);
            }
            else
            {
                stringOfs=get32(buf,4);
                if(stringOfs<4)
                {
                    printf("Invalid COFF object file bad symbol location (StringOfs = %d)\n", stringOfs);
                    exit(1);
                }
                stringOfs-=4;
                if(stringOfs>=stringSize)
                {
                    printf("Invalid COFF object file bad symbol location\n");
                    exit(1);
                }
                sym[i].name=(char *)&stringList[0]+stringOfs;
            }
	    if(!case_sensitive)
	    {
		strupr(sym[i].name);
	    }

	    sym[i].value=get32(buf, 8);
	    sym[i].section=get16(buf, 12);
	    sym[i].type=get16(buf, 14);
            sym[i].coff_class=buf[16];
            sym[i].extnum=-1;
	    sym[i].numAuxRecs=buf[17];
	    sym[i].isComDat=FALSE;

            switch(sym[i].coff_class)
            {
	    case COFF_SYM_SECTION: { /* section symbol */
                if(sym[i].section<-1)
                {
                    break;
                }
                /* section symbols declare an extern always, so can use in relocs */
                /* they may also include a PUBDEF */
		EXTREC ext;
                ext.name=sym[i].name;
                ext.pubdef=NULL;
                ext.modnum=0;
                ext.flags=EXT_NOMATCH;
                sym[i].extnum=externs.size();
		externs.push_back(ext);
                if(sym[i].section!=0) /* if the section is defined here, make public */
                {
		    pubdef=new PUBLIC();
		    pubdef->grpnum=-1;
		    pubdef->typenum=0;
		    pubdef->modnum=0;
		    pubdef->aliasName="";
		    pubdef->ofs=sym[i].value;

		    if(sym[i].section==-1)
		    {
			pubdef->seg=0;
		    }
		    else
		    {
			pubdef->seg=seglist[minseg+sym[i].section-1];
		    }
		    if((listnode=publics.find(sym[i].name)) != publics.end())
		    {
			for(j=0;j<listnode->second.size();++j)
			{
			    if(((PPUBLIC)listnode->second[j])->modnum==pubdef->modnum)
			    {
				if(((PPUBLIC)listnode->second[j])->aliasName == "")
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
			if(it == publics.end()) {
			    std::vector<void *> vv;
			    vv.push_back((void *)pubdef);
			    publics.insert(std::make_pair(sym[i].name,vv));
			} else {
			    it->second.push_back((void *)pubdef);
			}
		    }
                }
	    }
	    case COFF_SYM_STATIC: /* allowed, but ignored for now as we only want to process if required */
	    case COFF_SYM_LABEL:
            case COFF_SYM_FILE:
            case COFF_SYM_FUNCTION:
            case COFF_SYM_EXTERNAL:
                break;
            default:
                printf("unsupported symbol class %02X for symbol %s\n",sym[i].coff_class,sym[i].name.c_str());
                exit(1);
            }
	    if(sym[i].numAuxRecs)
	    {
		sym[i].auxRecs.resize(sym[i].numAuxRecs*PE_SYMBOL_SIZE);
	    }
	    else
	    {
		sym[i].auxRecs.resize(0);
	    }
	    
	    /* read in the auxillary records for this symbol */
            for(j=0;j<sym[i].numAuxRecs;j++)
            {
                if(fread(&sym[i].auxRecs[0]+j*PE_SYMBOL_SIZE,
			 1,PE_SYMBOL_SIZE,objfile)!=PE_SYMBOL_SIZE)
                {
                    printf("Invalid COFF object file\n");
                    exit(1);
                }
		sym[i+j+1].name="";
		sym[i+j+1].numAuxRecs=0;
		sym[i+j+1].value=0;
		sym[i+j+1].section=-1;
		sym[i+j+1].type=0;
		sym[i+j+1].coff_class=0;
		sym[i+j+1].extnum=-1;
            }
	    i+=j;
        }
    }
    for(i=0;i<numSect;i++)
    {
        fseek(objfile,fileStart+headerSize+i*PE_OBJECTENTRY_SIZE,
	      SEEK_SET);
        if(fread(buf,1,PE_OBJECTENTRY_SIZE,objfile)!=PE_OBJECTENTRY_SIZE)
        {
	    printf("Invalid COFF object file, unable to read section header\n");
	    exit(1);
        }
        /* virtual size is also the offset of the data into the segment */
	/*
	  if(buf[PE_OBJECT_VIRTSIZE]|buf[PE_OBJECT_VIRTSIZE+1]|buf[PE_OBJECT_VIRTSIZE+2]
	  |buf[PE_OBJECT_VIRTSIZE+3])
	  {
	  printf("Invalid COFF object file, section has non-zero virtual size\n");
	  exit(1);
	  }
	*/
        buf[8]=0; /* null terminate name */
        /* get shift value for relocs */
	relshift=get32(buf, PE_OBJECT_VIRTADDR);

        if(buf[0]=='/')
        {
            sectname=strtoul((char *)buf+1,(char**)&bigbuf,10);
            if(*bigbuf)
            {
                printf("Invalid COFF object file, invalid number %s\n",buf+1);
                exit(1);
            }
            if(sectname<4)
            {
                printf("Invalid COFF object file\n");
                exit(1);
            }
            sectname-=4;
            if(sectname>=stringSize)
            {
                printf("Invalid COFF object file\n");
                exit(1);
            }
	    nlist.push_back((char *)&stringList[0]+sectname);
	    sectname=nlist.size()-1;
        }
        else
        {
            nlist.push_back((char *)buf);
            sectname=nlist.size()-1;
        }
	size_t dollar = nlist[sectname].find('$');
        if(dollar != std::string::npos)
        {
            /* if we have a grouped segment, sort by original name */
            sectorder=sectname;
            /* and get real name, without $ sort section */
	    sectname=nlist.size();
	    std::string no_dollar = nlist[sectname].substr(dollar);
            nlist.push_back(no_dollar);
        }
        else
        {
            sectorder=-1;
        }

	numrel=get16(buf, PE_OBJECT_NUMREL);
	relofs=get32(buf, PE_OBJECT_RELPTR);

	PSEG seg = new SEG();
        seg->name=nlist[sectname];
        seg->orderindex=sectorder;
        seg->classindex=-1;
        seg->overlayindex=-1;
	seg->length=get32(buf, PE_OBJECT_RAWSIZE);

        seg->attr=SEG_PUBLIC|SEG_USE32;
        seg->winFlags=get32(buf, PE_OBJECT_FLAGS);
	seg->setbase(get32(buf, PE_OBJECT_RAWPTR));

	if(seg->winFlags & WINF_ALIGN_NOPAD)
	{
	    seg->winFlags &= (0xffffffff-WINF_ALIGN);
	    seg->winFlags |= WINF_ALIGN_BYTE;
	}
	
        switch(seg->winFlags & WINF_ALIGN)
        {
        case WINF_ALIGN_BYTE:
	    seg->attr |= SEG_BYTE;
	    break;
        case WINF_ALIGN_WORD:
	    seg->attr |= SEG_WORD;
	    break;
        case WINF_ALIGN_DWORD:
	    seg->attr |= SEG_DWORD;
	    break;
        case WINF_ALIGN_8:
	    seg->attr |= SEG_8BYTE;
	    break;
        case WINF_ALIGN_PARA:
	    seg->attr |= SEG_PARA;
	    break;
        case WINF_ALIGN_32:
	    seg->attr |= SEG_32BYTE;
	    break;
        case WINF_ALIGN_64:
	    seg->attr |= SEG_64BYTE;
	    break;
        case 0:
	    seg->attr |= SEG_PARA; /* default */
	    break;
        default:
	    printf("Invalid COFF object file, bad section alignment %08X\n",seg->winFlags);
	    exit(1);
        }

	/* invert all negative-logic flags */
        seg->winFlags ^= WINF_NEG_FLAGS;
	/* remove .debug sections */
	if(nlist[sectname] == ".debug")
	{
	    seg->winFlags |= WINF_REMOVE;
	    seg->length=0;
	    numrel=0;
	}

	if(seg->winFlags & WINF_COMDAT)
	{
	    printf("COMDAT section %s\n",nlist[sectname].c_str());
	    comdat=new COMDATREC();
	    combineType=0;
	    comdat->linkwith=0;
	    for(j=0;j<numSymbols;j++)
	    {
		if(sym[j].name=="") continue;
		if(sym[j].section==(i+1))
		{
		    if(sym[j].numAuxRecs!=1)
		    {
			printf("Invalid COMDAT section reference\n");
			exit(1);
		    }
		    printf("Section %s ",sym[j].name.c_str());
		    combineType=sym[j].auxRecs[14];
		    comdat->linkwith=sym[j].auxRecs[12]+(sym[j].auxRecs[13]<<8)+minseg-1;
		    printf("Combine type %i ",sym[j].auxRecs[14]);
		    printf("Link alongside section %i",comdat->linkwith);
		    
		    break;
		}
	    }
	    if(j==numSymbols)
	    {
		printf("Invalid COMDAT section\n");
		exit(1);
	    }
	    for(j++;j<numSymbols;j++)
	    {
		if(sym[j].name == "") continue;
		if(sym[j].section==(i+1))
		{
		    if(sym[j].numAuxRecs)
		    {
			printf("Invalid COMDAT symbol\n");
			exit(1);
		    }
		    
		    printf("COMDAT Symbol %s\n",sym[j].name.c_str());
		    comdatsym=(char *)sym[j].name.c_str();
		    sym[j].isComDat=TRUE;
		    break;
		}
	    }
	    /* associative sections don't have a name */
	    if(j==numSymbols)
	    {
		if(combineType!=5)
		{
		    printf("\nInvalid COMDAT section\n");
		    exit(1);
		}
		else
		{
		    printf("\n");
		}
		comdatsym=""; /* dummy name */
	    }
	    comdat->segnum=seglist.size();
	    comdat->combineType=combineType;

	    printf("COMDATs not yet supported\n");
	    exit(1);
	    
			printf("Combine types for duplicate COMDAT symbol %s do not match\n",comdatsym);
			exit(1);
	}

        if(seg->length)
        {
            seg->data.resize(seg->length);

            seg->datmask.resize((seg->length+7)/8);

            if(seg->getbase())
            {
                fseek(objfile,fileStart+seg->getbase(),SEEK_SET);
                if(fread(&seg->data[0],1,seg->length,objfile)
		   !=seg->length)
                {
		    printf("Invalid COFF object file\n");
		    exit(1);
                }
                for(j=0;j<(seg->length+7)/8;j++)
		    seg->datmask[j]=0xff;
            }
            else
            {
                for(j=0;j<(seg->length+7)/8;j++)
		    seg->datmask[j]=0;
            }

        }
        else
        {
	    seg->data.resize(0);
	    seg->datmask.resize(0);
        }

        if(numrel) fseek(objfile,fileStart+relofs,SEEK_SET);
        for(j=0;j<numrel;j++)
        {
	    if(fread(buf,1,PE_RELOC_SIZE,objfile)!=PE_RELOC_SIZE)
	    {
		printf("Invalid COFF object file, unable to read reloc table\n");
		exit(1);
	    }
	    PRELOC reloc = new RELOC();
	    /* get address to relocate */
	    reloc->ofs=buf[0]+(buf[1]<<8)+(buf[2]<<16)+(buf[3]<<24);
	    reloc->ofs-=relshift;
	    /* get segment */
	    reloc->seg=seglist[i+minseg];
	    reloc->disp=0;
	    /* get relocation target external index */
	    reloc->target=get32(buf, 4);
	    if(reloc->target>=numSymbols)
	    {
		printf("Invalid COFF object file, undefined symbol\n");
		exit(1);
	    }
	    k=reloc->target;
	    reloc->ttype=REL_EXTONLY; /* assume external reloc */
	    if(sym[k].extnum<0)
	    {
		switch(sym[k].coff_class)
		{
		case COFF_SYM_EXTERNAL: {
		    /* global symbols declare an extern when used in relocs */
		    EXTREC ext;
		    ext.name=(char *)sym[k].name.c_str();
		    ext.pubdef=NULL;
		    ext.modnum=0;
		    ext.flags=EXT_NOMATCH;
		    sym[k].extnum=externs.size();
		    externs.push_back(ext);
		    /* they may also include a COMDEF or a PUBDEF */
		    /* this is dealt with after all sections loaded, to cater for COMDAT symbols */
		} break;
		case COFF_SYM_STATIC: /* static symbol */
		case COFF_SYM_LABEL: /* code label symbol */
		    if(sym[k].section<-1)
		    {
			printf("cannot relocate against a debug info symbol\n");
			exit(1);
			break;
		    }
		    if(sym[k].section==0)
		    {
			if(sym[k].value)
			{
			    EXTREC ext;
			    ext.name=(char *)sym[k].name.c_str();
			    ext.pubdef=NULL;
			    ext.modnum=modname.size()-1;
			    ext.flags=EXT_NOMATCH;
			    sym[k].extnum=externs.size();
			    externs.push_back(ext);

			    PCOMREC com = new COMREC();
			    com->length=sym[k].value;
			    com->isFar=FALSE;
			    com->name=(char *)sym[k].name.c_str();
			    com->modnum=modname.size()-1;
			    comdefs.push_back(com);
			}
			else
			{
			    printf("Undefined symbol %s\n",sym[k].name.c_str());
			    exit(1);
			}
		    }
		    else
		    {
			/* update relocation information to reflect symbol */
			reloc->ttype=REL_SEGDISP;
			reloc->disp=sym[k].value;
			if(sym[k].section==-1)
			{
			    /* absolute symbols have section=-1 */
			    reloc->target=-1;
			}
			else
			{
			    /* else get real number of section */
			    reloc->target=sym[k].section+minseg-1;
			}
		    }
		    break;
		default:
		    printf("undefined symbol class 0x%02X for symbol %s\n",sym[k].coff_class,sym[k].name.c_str());
		    exit(1);
		}
	    }
	    if(reloc->ttype==REL_EXTONLY)
	    {
		/* set relocation target to external if sym is external */
		reloc->target=sym[k].extnum;
	    }
	    
	    /* frame is current segment (only relevant for non-FLAT output) */
	    reloc->ftype=REL_SEGFRAME;
	    reloc->frame=i+minseg;
	    /* set relocation type */
	    switch(get16(buf,8))
	    {
	    case COFF_FIX_DIR32:
		reloc->rtype=FIX_OFS32;
		break;

	    case COFF_FIX_RVA32:
		reloc->rtype=FIX_RVA32;
		break;
/* 
  case 0x0a: - 
  break;
  case 0x0b:
  break;
*/
	    case COFF_FIX_REL32:
		reloc->rtype=FIX_SELF_OFS32;
		break;
	    default:
		printf("unsupported COFF relocation type %04X\n",buf[8]+(buf[9]<<8));
		exit(1);
	    }
	    relocs.push_back(reloc);
        }

	seglist.push_back(seg);
    }
    /* build PUBDEFs or COMDEFs for external symbols defined here that aren't COMDAT symbols. */
    for(i=0;i<numSymbols;i++)
    {
	printf("sym[%d].section %d (%s)\n", i, sym[i].section, sym[i].name.c_str());
	if(sym[i].coff_class!=COFF_SYM_EXTERNAL) continue;
	if(sym[i].isComDat) continue;
	if(sym[i].section<-1)
	{
	    break;
	}
	if(sym[i].section==0)
	{
	    if(sym[i].value)
	    {
		PCOMREC com = new COMREC();
		com->length=sym[i].value;
		com->isFar=FALSE;
		com->name=sym[i].name;
		com->modnum=0;
		comdefs.push_back(com);
	    }
	}
	else
	{
	    pubdef=new PUBLIC();
	    pubdef->grpnum=-1;
	    pubdef->typenum=0;
	    pubdef->modnum=0;
	    pubdef->aliasName="";
	    pubdef->ofs=sym[i].value;
		
	    if(sym[i].section==-1)
	    {
		pubdef->seg=0;
	    }
	    else
	    {
		pubdef->seg=seglist[minseg+sym[i].section-1];
	    }
	    if((listnode=publics.find(sym[i].name)) != publics.end())
	    {
		for(j=0;j<listnode->second.size();++j)
		{
		    if(((PPUBLIC)listnode->second[j])->modnum==pubdef->modnum)
		    {
			if(((PPUBLIC)listnode->second[j])->aliasName == "")
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
    }
}

void loadCoffImport(FILE *objfile)
{
    UINT fileStart;
    UINT thiscpu;
    
    fileStart=ftell(objfile);

    if(fread(&buf[0],1,20,objfile)!=20)
    {
        printf("Unable to read from file\n");
        exit(1);
    }

    if(get32(buf,0) != 0xffff0000)
    {
	printf("Invalid Import entry\n");
	exit(1);
    }
    /* get CPU type */
    thiscpu=get16(buf,6);
    printf("Import CPU=%04X\n",thiscpu);
    
    if(thiscpu!=0x14c && thiscpu!=0x1f0)
    {
        printf("Unsupported CPU type for module (type %x)\n", thiscpu);
        exit(1);
    }
    
}
