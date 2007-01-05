#include "alink.h"
#include <assert.h>

static unsigned char defaultStub[]={
    0x4D,0x5A,0x6C,0x00,0x01,0x00,0x00,0x00,
    0x04,0x00,0x11,0x00,0xFF,0xFF,0x03,0x00,
    0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0E,0x1F,0xBA,0x0E,0x00,0xB4,0x09,0xCD,
    0x21,0xB8,0x00,0x4C,0xCD,0x21,0x54,0x68,
    0x69,0x73,0x20,0x70,0x72,0x6F,0x67,0x72,
    0x61,0x6D,0x20,0x72,0x65,0x71,0x75,0x69,
    0x72,0x65,0x73,0x20,0x57,0x69,0x6E,0x33,
    0x32,0x0D,0x0A,0x24,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static UINT defaultStubSize=sizeof(defaultStub);

void GetFixupTarget(PRELOC r,PSEG *bseg,UINT *tofs,int isFlat)
{
    PSEG baseseg = 0;
    PSEG targseg = 0;
    UINT targofs = 0;
    UINT frameaddr = 0;

    if( r->seg ) 
	r->outputPos=r->seg->getbase()+r->ofs;

    switch(r->ftype)
    {
    case REL_SEGFRAME:
    case REL_LILEFRAME:
	baseseg=r->seg;
	frameaddr = r->outputPos;
	break;
    case REL_GRPFRAME:
	baseseg=r->seg;
	break;
    case REL_EXTFRAME:
        switch(externs[r->frame].flags)
        {
        case EXT_MATCHEDPUBLIC:
	    if(!externs[r->frame].pubdef) 
	    {
		printf("Reloc:Unmatched extern %s\n",externs[r->frame].name.c_str());
		errcount++;
		break;
	    }
	    
            baseseg=externs[r->frame].pubdef->seg;
	    r->seg = baseseg;
	    frameaddr = externs[r->frame].pubdef->ofs + r->seg->getbase();
            break;
        case EXT_MATCHEDIMPORT:
            baseseg=impdefs[externs[r->frame].impnum].seg;
	    r->seg = baseseg;
            break;
        default:
            errcount++;
	    exit(1);
            break;
        }
        break;
    default:
        printf("Reloc:Unsupported FRAME type %i\n",r->ftype);
        errcount++;
	exit(1);
    }
    if(!baseseg)
    {
        printf("Undefined base seg\n");
        exit(1);
    }   /* this is a fix for TASM FLAT model, where FLAT group has no segments */

    switch(r->ttype)
    {
    case REL_EXTDISP:
    case REL_EXTONLY:
        switch(externs[r->target].flags)
        {
        case EXT_MATCHEDPUBLIC:
	    if(!externs[r->target].pubdef) 
	    {
		printf("Reloc:Unmatched extern %s\n",externs[r->frame].name.c_str());
		errcount++;
		break;
	    }
	    
            targseg=externs[r->target].pubdef->seg;
            targofs=externs[r->target].pubdef->ofs;
	    r->seg = targseg;
            break;
        case EXT_MATCHEDIMPORT:
            targseg=impdefs[externs[r->target].impnum].seg;
            targofs=impdefs[externs[r->target].impnum].ofs;
	    r->seg = targseg;
            break;
        default:
            printf("Reloc:Unmatched external referenced in frame\n");
            errcount++;
	    targseg=seglist[0];
	    targofs=0;
            break;
        }
	targofs+=r->disp;
	break;
    case REL_SEGONLY:
	targseg=r->targseg;
	targofs=0;
	break;
    case REL_SEGDISP:
	targseg=r->targseg;
	targofs = r->disp;
	break;
    case REL_GRPONLY:
	targseg=grplist[r->target]->seg;
	targofs=0;
	break;
    case REL_GRPDISP:
	targseg=grplist[r->target]->seg;
	targofs=r->disp;
	break;
    default:
	printf("Reloc:Unsupported TARGET type %i\n",r->ttype);
	errcount++;
    }
    if(targseg<0)
    {
        printf("undefined seg\n");
        exit(1);
    }
    if(!errcount && !targseg)
    {
	printf("Reloc: no target segment\n");
	
        errcount++;
    }
    if(!errcount && !baseseg)
    {
	printf("reloc: no base segment\n");
	
        errcount++;
    }

    if(!errcount)
    {
/*
        if(((seglist[targseg]->attr&SEG_ALIGN)!=SEG_ABS) &&
           ((seglist[baseseg]->attr&SEG_ALIGN)!=SEG_ABS))
        {
            if(seglist[baseseg]->getbase()>seglist[targseg]->getbase())
            {
                printf("Reloc:Negative base address\n");
                errcount++;
            }
            targofs+=seglist[targseg]->getbase()-seglist[baseseg]->getbase();
        }
*/
        if((baseseg->attr&SEG_ALIGN)==SEG_ABS)
        {
            //printf("Warning: Reloc frame is absolute segment\n");
            targseg=baseseg;
        }
        else if((targseg->attr&SEG_ALIGN)==SEG_ABS)
        {
            //printf("Warning: Reloc target is in absolute segment\n");
            targseg=baseseg;
        }
        if(!isFlat || ((baseseg->attr&SEG_ALIGN)==SEG_ABS))
        {
            if(baseseg->getbase()>(targseg->getbase()+targofs))
            {
                printf("Error: target address out of frame\n");
                printf("Base=%08X,target=%08X\n",
		       baseseg->getbase(),targseg->getbase()+targofs);
                errcount++;
            }
            targofs+=targseg->getbase()-baseseg->getbase();
            *bseg=baseseg;
            *tofs=targofs;
        }
        else
        {
            *bseg=0;
	    *tofs=targofs+targseg->getbase();
	    if(r->rtype == FIX_REL32 )
		*tofs -= frameaddr;
        }
    }
    else
    {
	printf("relocation error occurred\n");
        *bseg=0;
        *tofs=0;
	exit(1);
    }
}


void OutputCOMfile(const std::string &outname)
{
    long i,j;
    UINT started;
    UINT lastout;
    PSEG targseg;
    UINT targofs;
    FILE*outfile;
    PSEG seg;
    unsigned short temps;
    unsigned long templ;

    if(impsreq)
    {
        ReportError(ERR_ILLEGAL_IMPORTS);
    }

    errcount=0;
    if(gotstart)
    {
        GetFixupTarget(&startaddr,&seg,&startaddr.ofs,FALSE);
        if(errcount)
        {
            printf("Invalid start address record\n");
            exit(1);
        }

        if((startaddr.ofs+startaddr.seg->getbase())!=0x100)
        {
            printf("Warning, start address not 0100h as required for COM file\n");
        }
    }
    else
    {
        printf("Warning, no entry point specified\n");
    }

    for(i=0;i<relocs.size();i++)
    {
        GetFixupTarget(relocs[i],&targseg,&targofs,FALSE);
	seg = relocs[i]->seg;
        switch(relocs[i]->rtype)
        {
        case FIX_BASE:
        case FIX_PTR1616:
        case FIX_PTR1632:
             if((targseg->attr&SEG_ALIGN)!=SEG_ABS)
             {
                printf("Reloc %li:Segment selector relocations are not supported in COM files\n",i);
                errcount++;
             }
             else
             {
                j=relocs[i]->ofs;
                if(relocs[i]->rtype==FIX_PTR1616)
                {
                    if(targofs>0xffff)
                    {
                        printf("Relocs %li:Offset out of range\n",i);
                        errcount++;
                    }
                    temps=get16(seg->data,j);
                    temps+=targofs;
		    temps+=targseg->getbase()&0xf; /* non-para seg */
		    put16(seg->data,j,temps);
                    j+=2;
                }
                else if(relocs[i]->rtype==FIX_PTR1632)
                {
		    templ=get32(seg->data,j);
                    templ+=targofs;
		    templ+=targseg->getbase()&0xf; /* non-para seg */
		    put32(seg->data,j,templ);
                    j+=4;
                }
		temps = get16(seg->data,j);
                temps+=targseg->absframe;
		put16(seg->data,j,temps);
             }
             break;
        case FIX_OFS32:
        case FIX_OFS32_2:
	    templ=get32(seg->data,relocs[i]->ofs);
            templ+=targofs;
	    templ+=targseg->getbase()&0xf; /* non-para seg */
	    put32(seg->data,relocs[i]->ofs,templ);
             break;
        case FIX_OFS16:
        case FIX_OFS16_2:
             if(targofs>0xffff)
             {
                 printf("Relocs %li:Offset out of range\n",i);
                 errcount++;
             }
	     temps=get16(seg->data, relocs[i]->ofs);
	     temps+=targofs;
	     temps+=targseg->getbase()&0xf; /* non-para seg */
	     put16(seg->data, relocs[i]->ofs, temps);
             break;
        case FIX_LBYTE:
             seg->data[relocs[i]->ofs]+=targofs&0xff;
	     seg->data[relocs[i]->ofs]+=targseg->getbase()&0xf; /* non-para seg */
             break;
        case FIX_HBYTE:
	    templ=targofs+(targseg->getbase()&0xf); /* non-para seg */
	    seg->data[relocs[i]->ofs]+=(templ>>8)&0xff;
	    break;
        case FIX_SELF_LBYTE:
            if((targseg->attr&SEG_ALIGN)==SEG_ABS)
            {
                printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
                errcount++;
            }
            else
            {
                j=targseg->getbase()+targofs;
                j-=seg->getbase()+relocs[i]->ofs+1;
                if((j<-128)||(j>127))
                {
                    printf("Error: Reloc %li out of range\n",i);
                }
                else
                {
                    seg->data[relocs[i]->ofs]+=j;
                }
            }
            break;
        case FIX_SELF_OFS16:
        case FIX_SELF_OFS16_2:
            if((targseg->attr&SEG_ALIGN)==SEG_ABS)
            {
                printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
                errcount++;
            }
            else
            {
                j=targseg->getbase()+targofs;
                j-=seg->getbase()+relocs[i]->ofs+2;
                if((j<-32768)||(j>32767))
                {
                    printf("Error: Reloc %li out of range\n",i);
                }
                else
                {
		    temps=get16(seg->data, relocs[i]->ofs);
                    temps+=j;
		    put16(seg->data, relocs[i]->ofs, temps);
                }
            }
            break;
        case FIX_SELF_OFS32:
        case FIX_SELF_OFS32_2:
            if((targseg->attr&SEG_ALIGN)==SEG_ABS)
            {
                printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
                errcount++;
            }
            else
            {
                j=targseg->getbase()+targofs;
                j-=seg->getbase()+relocs[i]->ofs+4;
		templ=get32(seg->data, relocs[i]->ofs);
                templ+=j;
		put32(seg->data, relocs[i]->ofs, templ);
            }
            break;
        default:
                printf("Reloc %li:Relocation type %i not supported\n",i,relocs[i]->rtype);
                errcount++;
        }
    }

    if(errcount!=0)
    {
        exit(1);
    }
    outfile=fopen(outname.c_str(),"wb");
    if(!outfile)
    {
        printf("Error writing to file %s\n",outname.c_str());
        exit(1);
    }

    started=lastout=0;

    for(i=0;i<outlist.size();i++)
    {
        if(outlist[i] && ((outlist[i]->attr&SEG_ALIGN) !=SEG_ABS))
        {
            if(started>outlist[i]->getbase())
            {
                printf("Segment overlap\n");
                fclose(outfile);
                exit(1);
            }
            if(padsegments)
            {
                while(started<outlist[i]->getbase())
                {
                    fputc(0,outfile);
                    started++;
                }
            }
            else
            {
                started=outlist[i]->getbase();
            }
            for(j=0;j<outlist[i]->length;j++)
            {
                if(started>=0x100)
                {
                    if(GetNbit(outlist[i]->datmask,j))
                    {
                        for(;lastout<started;lastout++)
                        {
                            fputc(0,outfile);
                        }
                        fputc(outlist[i]->data[j],outfile);
                        lastout=started+1;
                    }
                    else if(padsegments)
                    {
                        fputc(0,outfile);
                        lastout=started+1;
                    }
                }
                else
                {
                    lastout=started+1;
                    if(GetNbit(outlist[i]->datmask,j))
                    {
                        printf("Warning - data at offset %08lX (%s:%08lX) discarded\n",started,outlist[i]->name.c_str(),j);
                    }
                }
                started++;
            }
        }
    }

    fclose(outfile);
}

void OutputEXEfile(const std::string &outname)
{
    long i,j;
    UINT started,lastout;
    PSEG targseg;
    UINT targofs;
    FILE*outfile;
    std::vector<UCHAR> headbuf;
    long relcount;
    int gotstack;
    UINT totlength;
    PSEG seg;
    unsigned short temps;
    unsigned long templ;

    if(impsreq)
    {
        ReportError(ERR_ILLEGAL_IMPORTS);
    }

    errcount=0;
    gotstack=0;
    headbuf.resize(0x40 + (4*relocs.size()));
    relcount=0;

    for(i=0;i<0x40;i++)
    {
        headbuf[i]=0;
    }

    headbuf[0x00]='M'; /* sig */
    headbuf[0x01]='Z';
    put16(headbuf, 0x0c, maxalloc);
    headbuf[0x18]=0x40;

    if(gotstart)
    {
        GetFixupTarget(&startaddr,&seg,&startaddr.ofs,FALSE);
        if(errcount)
        {
            printf("Invalid start address record\n");
            exit(1);
        }

        i=startaddr.seg->getbase();
        startaddr.ofs+=i&0xf;
        i>>=4;
        if((startaddr.ofs>65535)||(i>65535)||((startaddr.seg->attr&SEG_ALIGN)==SEG_ABS))
        {
            printf("Invalid start address\n");
            errcount++;
        }
        else
        {
	    put16(headbuf, 0x14, startaddr.ofs);
	    put16(headbuf, 0x16, i);
        }
    }
    else
    {
        printf("Warning, no entry point specified\n");
    }

    totlength=0;

    for(i=0;i<outlist.size();i++)
    {
        if((outlist[i]->attr&SEG_ALIGN)!=SEG_ABS)
        {
            totlength=outlist[i]->getbase()+outlist[i]->length;
            if((outlist[i]->attr&SEG_COMBINE)==SEG_STACK)
            {
                if(gotstack)
                {
                    printf("Internal error - stack segments not combined\n");
                    exit(1);
                }
                gotstack=1;
                if((outlist[i]->length>65536)||(outlist[i]->length==0))
                {
                    printf("SP value out of range\n");
                    errcount++;
                }
                if((outlist[i]->getbase()>0xfffff) || ((outlist[i]->getbase()&0xf)!=0))
                {
                    printf("SS value out of range\n");
                    errcount++;
                }
                if(!errcount)
                {
		    put16(headbuf, 0x0e, outlist[i]->getbase() >> 4);
		    put16(headbuf, 0x10, outlist[i]->length);
                }
            }
        }
    }

    if(!gotstack)
    {
        printf("Warning - no stack\n");
    }

    for(i=0;i<relocs.size();i++)
    {
	seg = relocs[i]->seg;
        GetFixupTarget(relocs[i],&targseg,&targofs,FALSE);
        switch(relocs[i]->rtype)
        {
        case FIX_BASE:
        case FIX_PTR1616:
        case FIX_PTR1632:
            j=relocs[i]->ofs;
            if(relocs[i]->rtype==FIX_PTR1616)
            {
                if(targofs>0xffff)
                {
                    printf("Relocs %li:Offset out of range\n",i);
                    errcount++;
                }
		temps=get16(seg->data,j);
                temps+=targofs;
		temps+=targseg->getbase()&0xf; /* non-para seg */
		put16(seg->data,j,temps);
                j+=2;
            }
            else if(relocs[i]->rtype==FIX_PTR1632)
            {
		templ=get32(seg->data,j);
                templ+=targofs;
		templ+=targseg->getbase()&0xf; /* non-para seg */
		put32(seg->data,j,templ);
                j+=4;
            }
            if((targseg->attr&SEG_ALIGN)!=SEG_ABS)
            {
                if(targseg->getbase()>0xfffff)
                {
                    printf("Relocs %li:Segment base out of range\n",i);
                    errcount++;
                }
		temps=get16(seg->data,j);
                temps+=(targseg->getbase()>>4);
		put16(seg->data,j,temps);
                templ=seg->getbase()>>4;
		put16(headbuf,0x40+relcount*4+2,templ);
                templ=(seg->getbase()&0xf)+j;
		put16(headbuf,0x40+relcount+4,templ);
                relcount++;
            }
            else
            {
		temps=get16(seg->data,j);
                temps+=targseg->absframe;
		put16(seg->data,j,temps);
            }
            break;
        case FIX_OFS32:
        case FIX_OFS32_2:
	    templ=get32(seg->data,relocs[i]->ofs);
            templ+=targofs;
	    templ+=targseg->getbase()&0xf; /* non-para seg */
	    put32(seg->data, relocs[i]->ofs, templ);
	    break;
        case FIX_OFS16:
        case FIX_OFS16_2:
	    if(targofs>0xffff)
	    {
		printf("Relocs %li:Offset out of range\n",i);
		errcount++;
	    }
	    temps = get32(seg->data, relocs[i]->ofs);
	    temps+=targofs;
	    temps+=targseg->getbase()&0xf; /* non-para seg */
	    put32(seg->data, relocs[i]->ofs, temps);
	    break;
        case FIX_LBYTE:
	    seg->data[relocs[i]->ofs]+=targofs&0xff;
	    seg->data[relocs[i]->ofs]+=targseg->getbase()&0xf; /* non-para seg */
	    break;
        case FIX_HBYTE:
	    templ=targofs+(targseg->getbase()&0xf); /* non-para seg */
	    seg->data[relocs[i]->ofs]+=(templ>>8)&0xff;
	    break;
        case FIX_SELF_LBYTE:
            if((targseg->attr&SEG_ALIGN)==SEG_ABS)
            {
                printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
                errcount++;
            }
            else
            {
                j=targseg->getbase()+targofs;
                j-=seg->getbase()+relocs[i]->ofs+1;
                if((j<-128)||(j>127))
                {
                    printf("Error: Reloc %li out of range\n",i);
                }
                else
                {
                    seg->data[relocs[i]->ofs]+=j;
                }
            }
            break;
        case FIX_SELF_OFS16:
        case FIX_SELF_OFS16_2:
            if((targseg->attr&SEG_ALIGN)==SEG_ABS)
            {
                printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
                errcount++;
            }
            else
            {
                j=targseg->getbase()+targofs;
                j-=seg->getbase()+relocs[i]->ofs+2;
                if((j<-32768)||(j>32767))
                {
                    printf("Error: Reloc %li out of range\n",i);
                }
                else
                {
		    temps = get32(seg->data, relocs[i]->ofs);
                    temps+=j;
		    put32(seg->data, relocs[i]->ofs, temps);
                }
            }
            break;
        case FIX_SELF_OFS32:
        case FIX_SELF_OFS32_2:
            if((targseg->attr&SEG_ALIGN)==SEG_ABS)
            {
                printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
                errcount++;
            }
            else
            {
                j=targseg->getbase()+targofs;
                j-=seg->getbase()+relocs[i]->ofs+4;
		templ=get32(seg->data,relocs[i]->ofs);
                templ+=j;
		put32(seg->data,relocs[i]->ofs,templ);
            }
            break;
        default:
                printf("Reloc %li:Relocation type %i not supported\n",i,relocs[i]->rtype);
                errcount++;
        }
    }

    if(relcount>65535)
    {
        printf("Too many relocations\n");
        exit(1);
    }

    put32(headbuf, 0x06, relcount);
    i=relcount*4+0x4f;
    i>>=4;
    totlength+=i<<4;
    put32(headbuf, 0x08, i);
    i=totlength%512;
    put32(headbuf, 0x02, i);
    i=(totlength+0x1ff)>>9;
    if(i>65535)
    {
        printf("File too large\n");
        exit(1);
    }
    put32(headbuf, 0x04, i);

    if(errcount!=0)
    {
        exit(1);
    }

    outfile=fopen(outname.c_str(),"wb");
    if(!outfile)
    {
        printf("Error writing to file %s\n",outname.c_str());
        exit(1);
    }

    i=get32(headbuf, 8) * 16;
    if(fwrite(&headbuf[0],1,i,outfile)!=i)
    {
        printf("Error writing to file %s\n",outname.c_str());
        exit(1);
    }

    started=0;
    lastout=0;

    for(i=0;i<outlist.size();i++)
    {
        if(outlist[i] && ((outlist[i]->attr&SEG_ALIGN) !=SEG_ABS))
        {
            if(started>outlist[i]->getbase())
            {
                printf("Segment overlap\n");
                fclose(outfile);
                exit(1);
            }
            if(padsegments)
            {
                while(started<outlist[i]->getbase())
                {
                    fputc(0,outfile);
                    started++;
                }
            }
            else
            {
                started=outlist[i]->getbase();
            }
            for(j=0;j<outlist[i]->length;j++)
            {
                if(GetNbit(outlist[i]->datmask,j))
                {
                    for(;lastout<started;lastout++)
                    {
                        fputc(0,outfile);
                    }
                    fputc(outlist[i]->data[j],outfile);
                    lastout=started+1;
                }
                else if(padsegments)
                {
                    fputc(0,outfile);
                    lastout=started+1;
                }
                started++;
            }
        }
    }

    if(lastout!=started)
    {
        fseek(outfile,0,SEEK_SET);
        lastout+=(headbuf[8]+256*headbuf[9])<<4;
        i=lastout%512;
	put32(headbuf, 2, i);
        i=(lastout+0x1ff)>>9;
	put32(headbuf, 4, i);
        i=((totlength-lastout)+0xf)>>4;
        if(i>65535)
        {
            printf("Memory requirements too high\n");
        }
	put32(headbuf, 10, i);
        if(fwrite(&headbuf[0],1,12,outfile)!=12)
        {
            printf("Error writing to file\n");
            exit(1);
        }
    }
    fclose(outfile);
}

long createOutputSection(char *name,UINT winFlags)
{
    UINT j;
    PSEG seg = new SEG();
    
    outlist.push_back(seg);
    seglist.push_back(seg);
    seg->name = name;
    seg->classindex=-1;
    seg->overlayindex=-1;
    j=outlist[outlist.size()-1]->getbase()+outlist[outlist.size()-1]->length;
    j+=(objectAlign-1);
    j&=(0xffffffff-(objectAlign-1));
    seg->setbase(j);
    seg->length=0;
    seg->data.resize(0);
    seg->datmask.resize(0);
    seg->absofs=seglist.size()-1;
    seg->attr=SEG_BYTE | SEG_PRIVATE;
    seg->winFlags=winFlags ^ WINF_NEG_FLAGS;
    return outlist.size()-1;
}

void BuildPEImports(long impsectNum,PUCHAR objectTable)
{
    long i,j,k;
    std::vector<UINT> reqimps;
    std::vector<std::string> dllNames;
    UINT reqcount=0;
    std::vector<int> dllNumImps, dllImpsDone, dllImpNameSize;
    UINT dllCount=0,dllNameSize=0,namePos;
    PSEG impsect;
    UINT thunkPos,thunk2Pos,impNamePos;

    if(impsectNum<0) return;
    for(i=0;i<externs.size();i++)
    {
        if(externs[i].flags!=EXT_MATCHEDIMPORT) continue;
        for(j=0;j<reqcount;j++)
        {
            if(reqimps[j]==externs[i].impnum) break;
        }
        if(j!=reqcount) continue;
	reqimps.push_back(externs[i].impnum);
        for(j=0;j<dllCount;j++)
        {
            if(impdefs[externs[i].impnum].mod_name == dllNames[j]) break;
        }
        if(j==dllCount)
        {
	    dllNames.push_back(impdefs[externs[i].impnum].mod_name);
	    dllNumImps.push_back(0);
	    dllImpsDone.push_back(0);
	    dllImpNameSize.push_back(0);
            dllNameSize+=dllNames[dllCount].size()+1;
            if(dllNameSize&1) dllNameSize++;
            dllCount++;
        }
        dllNumImps[j]++;
        if(impdefs[externs[i].impnum].flags==0) /* import by name? */
        {
            dllImpNameSize[j]+=strlen(impdefs[externs[i].impnum].imp_name.c_str())+3;
            /* the +3 ensure room for 2-byte hint and null terminator */
            if(dllImpNameSize[j]&1) dllImpNameSize[j]++;
        }
    }

    if(!reqcount || !dllCount) return;

    objectTable+=PE_OBJECTENTRY_SIZE*impsectNum; 
    k = get32(objectTable, -PE_OBJECTENTRY_SIZE+20); /* point to import object entry */

    k+= get32(objectTable, -PE_OBJECTENTRY_SIZE+16);

    k+=fileAlign-1;
    k&=(0xffffffff-(fileAlign-1)); /* aligned */

    /* k is now physical location of this object */

    put32(objectTable, 20, k); /* store physical file offset */

    k = get32(objectTable, -PE_OBJECTENTRY_SIZE+12); /* get virtual start of prev object */
    k+= get32(objectTable, -PE_OBJECTENTRY_SIZE+8); /* add virtual length of prev object */

    /* store base address (RVA) of section */
    put32(objectTable, 12, k);

    impsect=outlist[impsectNum];
    impsect->setbase(k+imageBase); /* get base address of section */

    impsect->length=(dllCount+1)*PE_IMPORTDIRENTRY_SIZE+dllNameSize;
    if(impsect->length&3) impsect->length+=4-(impsect->length&3); /* align to 4-byte boundary */
    thunkPos=impsect->getbase()+impsect->length;
    for(j=0,i=0;j<dllCount;j++)
    {
        i+=dllNumImps[j]+1; /* add number of entries in DLL thunk table */
    }
    /* now i= number of entries in Thunk tables for all DLLs */
    /* get address of name tables, which follow thunk tables */
    impNamePos=thunkPos+i*2*4; /* 2 thunk tables per DLL, 4 bytes per entry */
    impsect->length+=i*2*4; /* update length of section too. */

    for(j=0,i=0;j<dllCount;j++)
    {
        i+=dllImpNameSize[j]; /* add size of import names and hints */
    }

    impsect->length+=i;

    impsect->data.resize(impsect->length);
    impsect->datmask.resize((impsect->length+7)/8);
    for(i=0;i<(impsect->length+7)/8;i++)
    {
        impsect->datmask[i]=0xff;
    }

    /* end of directory entries=name table */
    namePos=impsect->getbase()+(dllCount+1)*PE_IMPORTDIRENTRY_SIZE;
    for(i=0;i<dllCount;i++)
    {
        /* add directory entry */
        j=i*PE_IMPORTDIRENTRY_SIZE;
	put32(impsect->data, j, thunkPos-imageBase); /* address of first thunk table */
	put32(impsect->data, j+4, 0); /* zero out time stamp */
        put32(impsect->data, j+8, 0); /* zero out version number */
	put32(impsect->data, j+12, namePos-imageBase); /* address of DLL name */
        thunk2Pos=thunkPos+(dllNumImps[i]+1)*4; /* address of second thunk table */
	put32(impsect->data, j+16, thunk2Pos-imageBase); /* store it */

        /* add name to table */
        strcpy((char *)(&impsect->data[0])+namePos-impsect->getbase(),(char *)dllNames[i].c_str());
        namePos+=dllNames[i].size()+1;
        if(namePos&1)
        {
            impsect->data[namePos-impsect->getbase()]=0;
            namePos++;
        }
        /* add imported names to table */
        for(k=0;k<reqcount;k++)
        {
            if(impdefs[reqimps[k]].mod_name != dllNames[i]) continue;
            if(impdefs[reqimps[k]].flags==0)
            {
                /* store pointers to name entry in thunk tables */
		put32(impsect->data, thunkPos-impsect->getbase(), impNamePos-imageBase);
		put32(impsect->data, thunk2Pos-impsect->getbase(), impNamePos-imageBase);

                /* no hint */
		put16(impsect->data, impNamePos-impsect->getbase(), 0);
                impNamePos+=2;
                /* store name */
                strcpy((char *)(&impsect->data[0])+impNamePos-impsect->getbase(),
                       (char *)impdefs[reqimps[k]].imp_name.c_str());
                impNamePos+=strlen(impdefs[reqimps[k]].imp_name.c_str())+1;
                if(impNamePos&1)
                {
                    impsect->data[impNamePos-impsect->getbase()]=0;
                    impNamePos++;
                }
            }
            else
            {
                /* store ordinal number in thunk tables */
                j=impdefs[reqimps[k]].ordinal+PE_ORDINAL_FLAG;
		put32(impsect->data, thunkPos-impsect->getbase(), j);
		put32(impsect->data, thunk2Pos-impsect->getbase(), j);
            }
            impdefs[reqimps[k]].abs=impsect->absofs;
            impdefs[reqimps[k]].ofs=thunk2Pos-impsect->getbase();
            thunkPos+=4;
            thunk2Pos+=4;
        }
        /* zero out end of thunk tables */
	put32(impsect->data, thunkPos-impsect->getbase(), 0);
	put32(impsect->data, thunk2Pos-impsect->getbase(), 0);
        thunkPos=thunk2Pos+4;
    }
    /* zero out the final entry to mark the end of the table */
    j=i*PE_IMPORTDIRENTRY_SIZE;
    for(i=0;i<PE_IMPORTDIRENTRY_SIZE;i++,j++)
    {
        impsect->data[j]=0;
    }

    k=impsect->length;
    k+=objectAlign-1;
    k&=(0xffffffff-(objectAlign-1));
    impsect->virtualSize=k;
    put32(objectTable, 8, k); /* store virtual size (in memory) of segment */

    k=impsect->length;
    put32(objectTable, 16, k); /* store initialised data size */

    return;
}

int compar( const void *a, const void *b ) 
{
    PRELOC aa = *((PRELOC *)a);
    PRELOC bb = *((PRELOC *)b);

    return (aa->outputPos < bb->outputPos) ? -1 : (aa->outputPos > bb->outputPos) ? 1 : 0;
}

void BuildPERelocs(long relocSectNum,PUCHAR objectTable)
{
    int i,j,r;
    PSEG relocSect, seg;
    UINT curStartPos;
    UINT curBlockPos;
    UINT k;
    PSEG targseg;
    UINT targofs;
    unsigned long templ, tinit;
    unsigned short temps;

    /* do fixups */
    for(i=0;i<relocs.size();i++)
    {
	seg = relocs[i]->seg;
        GetFixupTarget(relocs[i],&targseg,&targofs,TRUE);

	assert(seg->length > relocs[i]->ofs+3);

        switch(relocs[i]->rtype)
        {
        case FIX_BASE:
        case FIX_PTR1616:
        case FIX_PTR1632:
             if(targseg<0)
             {
                printf("Reloc %li:Segment selector relocations are not supported in PE files\n",i);
		printf("rtype=%02X, frame=%04X, target=%04X, ftype=%02X, ttype=%02X\n",
		       relocs[i]->rtype,relocs[i]->frame,relocs[i]->target,relocs[i]->ftype,
		       relocs[i]->ttype);
		
                errcount++;
             }
             else
             {
                j=relocs[i]->ofs;
                if(relocs[i]->rtype==FIX_PTR1616)
                {
                    if(targofs>0xffff)
                    {
                        printf("Relocs %li:Warning 32 bit offset in 16 bit field\n",i);
                    }
                    targofs&=0xffff;
		    temps=get16(seg->data, j);
                    temps+=targofs;
		    put16(seg->data, j, temps);
                    j+=2;
                }
                else if(relocs[i]->rtype==FIX_PTR1632)
                {
		    templ = get32(seg->data, j);
                    templ+=targofs;
		    put32(seg->data, j, templ);
                    j+=4;
                }
		temps = get16(seg->data, j);
                temps+=targseg->absframe;
		put16(seg->data, j, temps);
             }
             break;
        case FIX_OFS32:
        case FIX_OFS32_2:
	    templ = get32(seg->data, relocs[i]->ofs);
            templ+=targofs;
	    put32(seg->data, relocs[i]->ofs, templ);
             break;
	case FIX_PPC_ADDR24:
	    templ = get32(seg->data, relocs[i]->ofs);
            tinit = templ; templ = targofs;
	    put32(seg->data, relocs[i]->ofs, templ);
	    break;
	case FIX_PPC_REL24:
	    templ = get32(seg->data, relocs[i]->ofs);
            tinit = templ; 
	    //templ += (targofs - seg->getbase() - relocs[i]->ofs) & 0x3ffffc;
	    //templ |= tinit & 3;
	    templ = (templ & 0xfc000003) | 
		(targofs - seg->getbase() - relocs[i]->ofs) & 0x3fffffc;
	    put32(seg->data, relocs[i]->ofs, templ);
	    break;
	case FIX_PPC_SECTOFF:
	    templ = get32(seg->data, relocs[i]->ofs);
            tinit = templ; templ =+ targofs - imageBase;
	    put32(seg->data, relocs[i]->ofs, templ);
	    break;
	case FIX_PPC_RVA16LO:
	    templ = get16(seg->data, relocs[i]->ofs);
            tinit = templ; templ = targofs;
	    put16(seg->data, relocs[i]->ofs, templ);
	    break;
	case FIX_PPC_RVA16HA:
	    templ = get16(seg->data, relocs[i]->ofs) << 16;
	    tinit = templ; templ = targofs;
	    /* High adjust */
	    if( templ & 0x8000 ) templ += 0x8000;
	    put16(seg->data, relocs[i]->ofs, templ >> 16);
	    break;
	case FIX_ADDR32:
	    templ = get32(seg->data, relocs[i]->ofs);
	    tinit = templ; templ = targofs;
	    put32(seg->data, relocs[i]->ofs, templ);
	    break;
	case FIX_RVA32:
	    templ = get32(seg->data, relocs[i]->ofs);
            tinit = templ; templ =targofs - imageBase;
	    put32(seg->data, relocs[i]->ofs, templ);
	    break;
	case FIX_REL32:
	    templ = get32(seg->data, relocs[i]->ofs);
            tinit = templ; templ=targofs;
	    put32(seg->data, relocs[i]->ofs, templ);
	    break;
        case FIX_OFS16:
        case FIX_OFS16_2:
            if(targofs>0xffff)
            {
                printf("Relocs %li:Warning 32 bit offset in 16 bit field\n",i);
            }
            targofs&=0xffff;
	    temps = get16(seg->data, relocs[i]->ofs);
            temps+=targofs;
	    put16(seg->data, relocs[i]->ofs, temps);
	    break;
        case FIX_LBYTE:
        case FIX_HBYTE:
            printf("Error: Byte relocations not supported in PE files\n");
            errcount++;
            break;
        case FIX_SELF_LBYTE:
            if(targseg>=0)
            {
                printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
                errcount++;
            }
            else
            {
                j=targofs;
                j-=(seg->getbase()+relocs[i]->ofs+1);
                if((j<-128)||(j>127))
                {
                    printf("Error: Reloc %li out of range\n",i);
                }
                else
                {
                    seg->data[relocs[i]->ofs]+=j;
                }
            }
            break;
        case FIX_SELF_OFS16:
        case FIX_SELF_OFS16_2:
            if(targseg>=0)
            {
                printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
                errcount++;
            }
            else
            {
                j=targofs;
                j-=(seg->getbase()+relocs[i]->ofs+2);
                if((j<-32768)||(j>32767))
                {
                    printf("Error: Reloc %li out of range\n",i);
                }
                else
                {
		    temps = get16(seg->data, relocs[i]->ofs);
                    temps+=j;
		    put16(seg->data, relocs[i]->ofs, temps);
                }
            }
            break;
        case FIX_SELF_OFS32:
        case FIX_SELF_OFS32_2:
            if(targseg>=0)
            {
                printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
                errcount++;
            }
            else
            {
                j=targofs;
                j-=(seg->getbase()+relocs[i]->ofs+4);
		templ = get32(seg->data, relocs[i]->ofs);
                templ+=j;
		put32(seg->data, relocs[i]->ofs, templ);
            }
            break;
        default:
                printf("Reloc %li:Relocation type %i not supported\n",i,relocs[i]->rtype);
                errcount++;
        }
    }

    /* get reloc section */
    relocSect=outlist[relocSectNum]; /* get section structure */

    /* sort relocations into order of increasing address */
    qsort(&relocs[0], relocs.size(), sizeof(relocs[0]), &compar);

    for(i=0,curStartPos=0,curBlockPos=0;i<relocs.size();i++)
    {
        switch(relocs[i]->rtype)
        {
        case FIX_SELF_OFS32:
        case FIX_SELF_OFS32_2:
        case FIX_SELF_OFS16:
        case FIX_SELF_OFS16_2:
	case FIX_SELF_LBYTE:
	case FIX_RVA32:
                continue; /* self-relative fixups and RVA fixups don't relocate */
        default:
                break;
        }
        if(relocs[i]->outputPos>=(curStartPos+0x1000)) /* more than 4K past block start? */
        {
            j=relocSect->length&3;
            if(j) /* unaligned block position */
            {
                relocSect->length+=4-j; /* update length to align block */
                /* and block memory */
                relocSect->data.resize(relocSect->length);
                /* update size of current reloc block */
		k = get32(relocSect->data, curBlockPos+4);
                k+=4-j;
		put32(relocSect->data, curBlockPos+4, k);
                for(j=4-j;j>0;j--)
                {
                    relocSect->data[relocSect->length-j]=0;
                }
            }
            curBlockPos=relocSect->length; /* get address in section of current block */
            relocSect->length+=8; /* 8 bytes block header */
            /* increase size of block */
            relocSect->data.resize(relocSect->length);
            /* store reloc base address, and block size */
            curStartPos=relocs[i]->outputPos&0xfffff000; /* start of mem page */

            /* start pos is relative to image base */
	    put32(relocSect->data, curBlockPos, curStartPos-imageBase);
	    put32(relocSect->data, curBlockPos+4, 8); /* start size is 8 bytes */
        }
        relocSect->data.resize(relocSect->length+2);

        j=relocs[i]->outputPos-curStartPos; /* low 12 bits of address */
        switch(relocs[i]->rtype)
        {
        case FIX_PTR1616:
        case FIX_OFS16:
        case FIX_OFS16_2:
            j|= PE_REL_LOW16;
            break;
        case FIX_PTR1632:
        case FIX_OFS32:
    case FIX_OFS32_2:
            j|= PE_REL_OFS32;
        }
        /* store relocation */
	put16(relocSect->data, relocSect->length, j);
        /* update block length */
        relocSect->length+=2;
        /* update size of current reloc block */
	k = get32(relocSect->data, curBlockPos+4);
        k+=2;
	put32(relocSect->data, curBlockPos+4, k);
    }
    /* if no fixups, then build NOP fixups, to make Windows NT happy */
    /* when it trys to relocate image */
    if(relocSect->length==0)
    {
	/* 12 bytes for dummy section */
        relocSect->length=12;
	relocSect->data.resize(12);
	/* zero it out for now */
        for(i=0;i<12;i++) relocSect->data[i]=0;
    relocSect->data[4]=12; /* size of block */
    }

    relocSect->datmask.resize((relocSect->length+7)/8);
    for(i=0;i<(relocSect->length+7)/8;i++)
    {
        relocSect->datmask[i]=0xff;
    }

    objectTable+=PE_OBJECTENTRY_SIZE*relocSectNum; /* point to reloc object entry */
    k=relocSect->length;
    k+=objectAlign-1;
    k&=(0xffffffff-(objectAlign-1));
    relocSect->virtualSize=k;
    put32(objectTable, 8, k); /* store virtual size (in memory) of segment */
    put32(objectTable, 16, relocSect->length); /* store initialised data size /*/

    k = get32(objectTable, -PE_OBJECTENTRY_SIZE+20);
    /* add physical length of prev object */
    k += get32(objectTable, -PE_OBJECTENTRY_SIZE+16);

    k+=fileAlign-1;
    k&=(0xffffffff-(fileAlign-1)); /* aligned */

    /* k is now physical location of this object */

    put32(objectTable, 20, k); /* store physical file offset */
    /* get virtual start of prev object */
    k = get32(objectTable, -PE_OBJECTENTRY_SIZE+12);
    k += get32(objectTable, -PE_OBJECTENTRY_SIZE+8);

    /* store base address (RVA) of section */
    put32(objectTable, 12, k);

    relocSect->setbase(k+imageBase); /* relocate section */

    return;
}

void BuildPEExports(long SectNum,PUCHAR objectTable,PUCHAR name)
{
        long i,j;
        UINT k;
    PSEG expSect;
    UINT namelen;
    UINT numNames=0;
    UINT RVAStart;
    UINT nameRVAStart;
    UINT ordinalStart;
    UINT nameSpaceStart;
    UINT minOrd;
    UINT maxOrd;
    UINT numOrds;
    std::vector<PEXPREC> nameList;
    PEXPREC curName;

    if(!expdefs.size() || (SectNum<0)) return; /* return if no exports */
    expSect=outlist[SectNum];

    if(name)
    {
        namelen=strlen((char *)name);
        /* search backwards for path separator */
        for(i=namelen-1;(i>=0) && (name[i]!=PATH_CHAR);i--);
        if(i>=0) /* if found path separator */
        {
            name+=(i+1); /* update name pointer past path */
            namelen -= (i+1); /* and reduce length */
        }
    }
    else namelen=0;

    expSect->length=PE_EXPORTHEADER_SIZE+4*expdefs.size()+namelen+1;
    /* min section size= header size + num exports * pointer size */
    /* plus space for null-terminated name */

    minOrd=0xffffffff; /* max ordinal num */
    maxOrd=0;

    for(i=0;i<expdefs.size();i++)
    {
        /* check we've got an exported name */
        if(expdefs[i].exp_name != "" && expdefs[i].exp_name.length())
        {
            /* four bytes for name pointer */
            /* two bytes for ordinal, 1 for null terminator */
            expSect->length+=expdefs[i].exp_name.length()+7;
            numNames++;
        }

        if(expdefs[i].flags&EXP_ORD) /* ordinal? */
        {
            if(expdefs[i].ordinal<minOrd) minOrd=expdefs[i].ordinal;
            if(expdefs[i].ordinal>maxOrd) maxOrd=expdefs[i].ordinal;
        }
    }

    numOrds=expdefs.size(); /* by default, number of RVAs=number of exports */
    if(maxOrd>=minOrd) /* actually got some ordinal references? */
    {
        i=maxOrd-minOrd+1; /* get number of ordinals */
        if(i>expdefs.size()) /* if bigger range than number of exports */
        {
            expSect->length+=4*(i-expdefs.size()); /* up length */
            numOrds=i; /* get new num RVAs */
        }
    }
    else
    {
        minOrd=1; /* if none defined, min is set to 1 */
    }

    expSect->data.resize(expSect->length);

    objectTable+=PE_OBJECTENTRY_SIZE*SectNum; /* point to reloc object entry */
    k=expSect->length;
    k+=objectAlign-1;
    k&=(0xffffffff-(objectAlign-1));
    expSect->virtualSize=k;
    put32(objectTable, 8, k);  /* store virtual size (in memory) of segment */
    k=expSect->length;
    put32(objectTable, 16, k); /* store initialised data size */

    k =get32(objectTable, -PE_OBJECTENTRY_SIZE+20); /* get physical start of prev object */
    k+=get32(objectTable, -PE_OBJECTENTRY_SIZE+16); /* add physical length of prev object */

    k+=fileAlign-1;
    k&=(0xffffffff-(fileAlign-1)); /* aligned */

    /* k is now physical location of this object */

    put32(objectTable, 20, k); /* store physical file offset */

    k = get32(objectTable, -PE_OBJECTENTRY_SIZE+12); /* get virtual start of prev object */
    k+= get32(objectTable, -PE_OBJECTENTRY_SIZE+8); /* add virtual length of prev object */

    /* store base address (RVA) of section */
    put32(objectTable, 12, k);

    expSect->setbase(k+imageBase); /* relocate section */

    /* start with buf=all zero */
    for(i=0;i<expSect->length;i++) expSect->data[i]=0;

    /* store creation time of export data */
    k=(UINT)time(NULL);
    put32(expSect->data, 4, k);
    put32(expSect->data, 16, minOrd); /* store ordinal base */

    /* store number of RVAs */
    put32(expSect->data, 20, numOrds);

    RVAStart=PE_EXPORTHEADER_SIZE; /* start address of RVA table */
    nameRVAStart=RVAStart+numOrds*4; /* start of name table entries */
    ordinalStart=nameRVAStart+numNames*4; /* start of associated ordinal entries */
    nameSpaceStart=ordinalStart+numNames*2; /* start of actual names */

    /* store number of named exports */
    put32(expSect->data, 24, numNames);

    /* store address of address table */
    put32(expSect->data, 28, RVAStart+expSect->getbase()-imageBase);

    /* store address of name table */
    put32(expSect->data, 32, nameRVAStart+expSect->getbase()-imageBase);

    /* store address of ordinal table */
    put32(expSect->data, 36, ordinalStart+expSect->getbase()-imageBase);

    /* process numbered exports */
    for(i=0;i<expdefs.size();i++)
    {
        if(expdefs[i].flags&EXP_ORD)
        {
            /* get current RVA */
	    k = get32(expSect->data, RVAStart+4*(expdefs[i].ordinal-minOrd));
            if(k) /* error if already used */
            {
                printf("Duplicate export ordinal %i\n",expdefs[i].ordinal);
                exit(1);
            }
            /* get RVA of export entry */
            k=expdefs[i].pubdef->ofs+
                expdefs[i].pubdef->seg->getbase()-
                imageBase;
            /* store it */
	    put32(expSect->data, RVAStart+4*(expdefs[i].ordinal-minOrd), k);
        }
    }

    /* process non-numbered exports */
    for(i=0,j=RVAStart;i<expdefs.size();i++)
    {
        if(!(expdefs[i].flags&EXP_ORD))
        {
            do
            {
		k = get32(expSect->data, j);
                if(k) j+=4;
            }
            while(k); /* move through table until we find a free spot */
            /* get RVA of export entry */
            k=expdefs[i].pubdef->ofs;
            k+=expdefs[i].pubdef->seg->getbase();
            k-=imageBase;
            /* store RVA */
	    put32(expSect->data, j, k);
            expdefs[i].ordinal=(j-RVAStart)/4+minOrd; /* store ordinal */
            j+=4;
        }
    }

    if(numNames) /* sort name table if present */
    {
        nameList.resize(numNames);
        j=0; /* no entries yet */
        for(i=0;i<expdefs.size();i++)
        {
            if(expdefs[i].exp_name != "")
            {
                /* make entry in name list */
                nameList[j]=&expdefs[i];
                j++;
            }
        }
        /* sort them into order */
        for(i=1;i<numNames;i++)
        {
            curName=nameList[i];
            for(j=i-1;j>=0;j--)
            {
                /* break out if we're above previous entry */
                if(curName->exp_name > nameList[j]->exp_name)
                {
                    break;
                }
                /* else move entry up */
                nameList[j+1]=nameList[j];
            }
            j++; /* move to one after better entry */
            nameList[j]=curName; /* insert current entry into position */
        }
        /* and store */
        for(i=0;i<numNames;i++)
        {
            /* store ordinal */
	    put16(expSect->data, ordinalStart, nameList[i]->ordinal-minOrd);
            ordinalStart+=2;
            /* store name RVA */
	    put32(expSect->data, nameRVAStart, nameSpaceStart+expSect->getbase()-imageBase);
            nameRVAStart+=4;
            /* store name */
            for(j=0;nameList[i]->exp_name[j];j++,nameSpaceStart++)
            {
                expSect->data[nameSpaceStart]=nameList[i]->exp_name[j];
            }
            /* store NULL */
            expSect->data[nameSpaceStart]=0;
            nameSpaceStart++;
        }
    }

    /* store library name */
    for(j=0;j<namelen;j++)
    {
        expSect->data[nameSpaceStart+j]=name[j];
    }
    if(namelen)
    {
        expSect->data[nameSpaceStart+j]=0;
        /* store name RVA */
	put32(expSect->data, 12, nameSpaceStart+expSect->getbase()-imageBase);
    }

    expSect->datmask.resize((expSect->length+7)/8);
    for(i=0;i<(expSect->length+7)/8;i++)
    {
        expSect->datmask[i]=0xff;
    }

    return;
}

void BuildPEResources(long sectNum,PUCHAR objectTable)
{
    long i,j;
    UINT k;
    SEG *ressect;
    RESOURCE curres;
    int numtypes,numnamedtypes;
    int numPairs,numnames,numids;
    UINT nameSize,dataSize;
    UINT tableSize,dataListSize;
    UINT namePos,dataPos,tablePos,dataListPos;
    UINT curTypePos,curNamePos,curLangPos;
    unsigned char *curTypeName,*curName;
    int curTypeId,curId;

    if((sectNum<0) || !resource.size()) return;

    objectTable+=PE_OBJECTENTRY_SIZE*sectNum; /* point to import object entry */
    k=get32(objectTable, -PE_OBJECTENTRY_SIZE+20); /* get physical start of prev object */
    k+=get32(objectTable, -PE_OBJECTENTRY_SIZE+16); /* add physical length of prev object */

    k+=fileAlign-1;
    k&=(0xffffffff-(fileAlign-1)); /* aligned */

    /* k is now physical location of this object */

    put32(objectTable, 20, k); /* store physical file offset */
    k = get32(objectTable, 12); /* get virtual start of prev object */
    k+= get32(objectTable, 8); /* add virtual length of prev object */

    /* store base address (RVA) of section */
    put32(objectTable, 12, k);

    ressect=outlist[sectNum];
    ressect->setbase(k); /* get base RVA of section */

    /* sort into type-id order */
    for(i=1;i<resource.size();i++)
    {
        curres=resource[i];
        for(j=i-1;j>=0;j--)
        {
            if(resource[j].xtypename.size())
            {
                if(!curres.xtypename.size()) break;
                if(_wstricmp(&curres.xtypename[0],&resource[j].xtypename[0])>0) break;
                if(_wstricmp(&curres.xtypename[0],&resource[j].xtypename[0])==0)
                {
                    if(resource[j].name.size())
                    {
                        if(!curres.name.size()) break;
                        if(_wstricmp(&curres.name[0],&resource[j].name[0])>0) break;
                        if(_wstricmp(&curres.name[0],&resource[j].name[0])==0)
                        {
                            if(resource[j].languageid>curres.languageid)
                                break;
                            if(resource[j].languageid==curres.languageid)
                            {
                                printf("Error duplicate resource ID\n");
                                exit(1);
                            }
                        }
                    }
                    else
                    {
                        if(!curres.name.size())
                        {
                            if(curres.id>resource[j].id) break;
                            if(curres.id==resource[j].id)
                            {
                                if(resource[j].languageid>curres.languageid)
                                    break;
                                if(resource[j].languageid==curres.languageid)
                                {
                                    printf("Error duplicate resource ID\n");
                                    exit(1);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if(!curres.xtypename.size())
                {
                    if(curres.typeid>resource[j].typeid) break;
                    if(curres.typeid==resource[j].typeid)
                    {
                        if(resource[j].name.size())
                        {
                            if(!curres.name.size()) break;
                            if(_wstricmp(&curres.name[0],&resource[j].name[0])>0) break;
                            if(_wstricmp(&curres.name[0],&resource[j].name[0])==0)
                            {
                                if(resource[j].languageid>curres.languageid)
                                    break;
                                if(resource[j].languageid==curres.languageid)
                                {
                                    printf("Error duplicate resource ID\n");
                                    exit(1);
                                }
                            }
                        }
                        else
                        {
                            if(!curres.name.size())
                            {
                                if(curres.id>resource[j].id) break;
                                if(curres.id==resource[j].id)
                                {
                                    if(resource[j].languageid>curres.languageid)
                                        break;
                                    if(resource[j].languageid==curres.languageid)
                                    {
                                        printf("Error duplicate resource ID\n");
                                        exit(1);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            resource[j+1]=resource[j];
        }
        j++;
        resource[j]=curres;
    }

    nameSize=0;
    dataSize=0;
    for(i=0;i<resource.size();i++)
    {
        if(resource[i].xtypename.size())
        {
            nameSize+=2+2*_wstrlen(&resource[i].xtypename[0]);
	}
        if(resource[i].name.size())
        {
            nameSize+=2+2*_wstrlen(&resource[i].name[0]);
        }
        dataSize+=resource[i].length+3;
	dataSize&=0xfffffffc;
    }

    /* count named types */
    numnamedtypes=0;
    numPairs=resource.size();
    for(i=0;i<resource.size();i++)
    {
        if(!resource[i].xtypename.size()) break;
        if((i>0) && !_wstricmp(&resource[i].xtypename[0],&resource[i-1].xtypename[0]))
        {
            if(resource[i].name.size())
            {
                if(_wstricmp(&resource[i].name[0],&resource[i-1].name[0])==0)
                    numPairs--;
            }
            else
            {
                if(!resource[i-1].name.size() && (resource[i].id ==resource[i-1].id))
                    numPairs--;
            }
            continue;
        }
        numnamedtypes++;
    }
    numtypes=numnamedtypes;
    for(;i<resource.size();i++)
    {
        if((i>0) && !resource[i-1].xtypename.size() && (resource[i].typeid==resource[i-1].typeid))
        {
            if(resource[i].name.size())
            {
                if(_wstricmp(&resource[i].name[0],&resource[i-1].name[0])==0)
                    numPairs--;
            }
            else
            {
                if(!resource[i-1].name.size() && (resource[i].id ==resource[i-1].id))
                    numPairs--;
            }
            continue;
        }
        numtypes++;
    }

    tableSize=(resource.size()+numtypes+numPairs)*PE_RESENTRY_SIZE+
        (numtypes+numPairs+1)*PE_RESDIR_SIZE;
    dataListSize=resource.size()*PE_RESDATAENTRY_SIZE;

    tablePos=0;
    dataListPos=tableSize;
    namePos=tableSize+dataListSize;
    dataPos=tableSize+nameSize+dataListSize+3;
    dataPos&=0xfffffffc;

    ressect->length=dataPos+dataSize;

    ressect->data.resize(ressect->length);

    ressect->datmask.resize((ressect->length+7)/8);

    /* empty section to start with */
    for(i=0;i<ressect->length;i++)
        ressect->data[i]=0;

    /* build master directory */
    /* store time/date of creation */
    k=(UINT)time(NULL);
    put32(ressect->data, 4, k);
    put32(ressect->data, 12, numnamedtypes);

    tablePos=16+numtypes*PE_RESENTRY_SIZE;
    curTypePos=16;
    curTypeName=NULL;
    curTypeId=-1;
    for(i=0;i<resource.size();i++)
    {
        if(!(resource[i].xtypename.size() && curTypeName &&
	     !_wstricmp(&resource[i].xtypename[0],curTypeName))
            && !(!resource[i].xtypename.size() && !curTypeName && curTypeId==resource[i].typeid))
        {
            if(resource[i].xtypename.size())
            {
		put32(ressect->data, curTypePos, namePos);
                curTypeName=&resource[i].xtypename[0];
                k=_wstrlen(curTypeName);
		put16(ressect->data, namePos, k);
                namePos+=2;
                memcpy(&ressect->data[0]+namePos,curTypeName,k*2);
                namePos+=k*2;
                curTypeId=-1;
            }
            else
            {
                curTypeName=NULL;
                curTypeId=resource[i].typeid;
		put32(ressect->data, curTypePos, curTypeId);
            }
	    put32(ressect->data, curTypePos+4, tablePos);

            numnames=numids=0;
            for(j=i;j<resource.size();j++)
            {
                if(resource[i].xtypename.size())
                {
                    if(!resource[j].xtypename.size()) break;
                    if(_wstricmp(&resource[i].xtypename[0],&resource[j].xtypename[0])!=0) break;
                }
                else
                {
                    if(resource[j].typeid!=resource[i].typeid) break;
                }
                if(resource[j].name.size())
                {
                    if(((j>i) && (_wstricmp(&resource[j].name[0],&resource[j-1].name[0])!=0))
                        || (j==i))
                        numnames++;
                }
                else
                {
                    if(((j>i) && (resource[j-1].name.size() || (resource[j].id!=resource[j-1].id)))
                        || (j==i))
                        numids++;
                }
            }
	    /* store time/date of creation */
	    k=(UINT)time(NULL);
	    put32(ressect->data, tablePos+4, k);
	    put16(ressect->data, tablePos+12, numnames);
	    put16(ressect->data, tablePos+14, numids);

            curNamePos=tablePos+PE_RESDIR_SIZE;
            curName=NULL;
            curId=-1;
            tablePos+=PE_RESDIR_SIZE+(numids+numnames)*PE_RESENTRY_SIZE;
            curTypePos+=PE_RESENTRY_SIZE;
        }
        if(!(resource[i].name.size() && curName &&
	     !_wstricmp(&resource[i].name[0],curName))
	   && !(!resource[i].name.size() && !curName && curId==resource[i].id))
        {
            if(resource[i].name.size())
            {
		put32(ressect->data, curNamePos, namePos);
                curName=&resource[i].name[0];
                k=_wstrlen(curName);
		put16(ressect->data, namePos, k);
                namePos+=2;
                memcpy(&ressect->data[0]+namePos,curName,k*2);
                namePos+=k*2;
                curId=-1;
            }
            else
            {
                curName=NULL;
                curId=resource[i].id;
		put32(ressect->data, curNamePos, curId);
            }
	    put32(ressect->data, curNamePos+4, 0x80000000 | tablePos);

            numids=0;
            for(j=i;j<resource.size();j++)
            {
                if(resource[i].xtypename.size())
                {
                    if(!resource[j].xtypename.size()) break;
                    if(_wstricmp(&resource[i].xtypename[0],&resource[j].xtypename[0])!=0) break;
                }
                else
                {
                    if(resource[j].typeid!=resource[i].typeid) break;
                }
                if(resource[i].name.size())
                {
                    if(!resource[j].name.size()) break;
                    if(_wstricmp(&resource[j].name[0],&resource[i].name[0])!=0) break;
                }
                else
                {
                    if(resource[j].id!=resource[i].id) break;
                }
                numids++;
            }
            numnames=0; /* no names for languages */
	    /* store time/date of creation */
	    k=(UINT)time(NULL);
	    put32(ressect->data, tablePos+4, k);
	    put16(ressect->data, tablePos+12, numnames);
	    put16(ressect->data, tablePos+14, numids);

            curLangPos=tablePos+PE_RESDIR_SIZE;
            tablePos+=PE_RESDIR_SIZE+numids*PE_RESENTRY_SIZE;
            curNamePos+=PE_RESENTRY_SIZE;
        }
	put32(ressect->data, curLangPos, resource[i].languageid);
	put32(ressect->data, curLangPos+4, dataListPos);
        curLangPos+=PE_RESENTRY_SIZE;

	put32(ressect->data, dataListPos, dataPos+ressect->getbase());
	put32(ressect->data, dataListPos+4, resource[i].length);
        memcpy(&ressect->data[0]+dataPos,&resource[i].data[0],resource[i].length);
        dataPos+=resource[i].length+3;
	dataPos&=0xfffffffc;
        dataListPos+=PE_RESDATAENTRY_SIZE;
    }

    /* mark whole section as required output */
    for(i=0;i<(ressect->length+7)/8;i++)
        ressect->datmask[i]=0xff;

    /* update object table */
    ressect->setbase(ressect->getbase()+imageBase);

    k=ressect->length;
    k+=objectAlign-1;
    k&=(0xffffffff-(objectAlign-1));
    ressect->virtualSize=k;
    put32(objectTable, 8, k); /* store virtual size (in memory) of segment */
    k=ressect->length;
    put32(objectTable, 16, k); /* store initialised data size */
    return;
}

void getStub(PUCHAR *pstubData,UINT *pstubSize)
{
    FILE *f;
    unsigned char headbuf[0x1c];
    std::vector<UCHAR> buf;
    UINT imageSize;
    UINT headerSize;
    UINT relocSize;
    UINT relocStart;
    int i;

    if(stubName != "")
    {
        f=fopen((char *)stubName.c_str(),"rb");
        if(!f)
        {
            printf("Unable to open stub file %s\n",stubName.c_str());
            exit(1);
        }
        if(fread(headbuf,1,0x1c,f)!=0x1c) /* try and read 0x1c bytes */
        {
            printf("Error reading from file %s\n",stubName.c_str());
            exit(1);
        }
        if((headbuf[0]!=0x4d) || (headbuf[1]!=0x5a))
        {
            printf("Stub not valid EXE file\n");
            exit(1);
        }
        /* get size of image */
        imageSize=headbuf[2]+(headbuf[3]<<8)+((headbuf[4]+(headbuf[5]<<8))<<9);
        if(imageSize%512) imageSize-=512;
        headerSize=(headbuf[8]+(headbuf[9]<<8))<<4;
        relocSize=(headbuf[6]+(headbuf[7]<<8))<<2;
        imageSize-=headerSize;
        printf("imageSize=%i\n",imageSize);
        printf("header=%i\n",headerSize);
        printf("reloc=%i\n",relocSize);

        /* allocate buffer for load image */
        buf.resize(imageSize+0x40+((relocSize+0xf)&0xfffffff0));
        /* copy header */
        for(i=0;i<0x1c;i++) buf[i]=headbuf[i];

        relocStart=headbuf[0x18]+(headbuf[0x19]<<8);
        /* load relocs */
        fseek(f,relocStart,SEEK_SET);
        if(fread(&buf[0x40],1,relocSize,f)!=relocSize)
        {
            printf("Error reading from file %s\n",stubName.c_str());
            exit(1);
        }

        /* paragraph align reloc size */
        relocSize+=0xf;
        relocSize&=0xfffffff0;

        /* move to start of data */
        fseek(f,headerSize,SEEK_SET);
        /* new header is 4 paragraphs long + relocSize*/
        relocSize>>=4;
        relocSize+=4;
	put16(buf, 8, relocSize);
        headerSize=relocSize<<4;
        /* load data into correct position */
        if(fread(&buf[headerSize],1,imageSize,f)!=imageSize)
        {
            printf("Error reading from file %s\n",stubName.c_str());
            exit(1);
        }
        /* relocations start at 0x40 */
	put16(buf, 0x18, 0x40);
        imageSize+=headerSize; /* total file size */
        /* store pointer and size */
        (*pstubData)=&buf[0];
        (*pstubSize)=imageSize;
        i=imageSize%512; /* size mod 512 */
        imageSize=(imageSize+511)>>9; /* number of 512-byte pages */
	put16(buf, 2, i);
	put16(buf, 4, imageSize);
    }
    else
    {
        (*pstubData)=defaultStub;
        (*pstubSize)=defaultStubSize;
    }
}

void OutputWin32file(const std::string &outname)
{
    long i,j,k;
    UINT started;
    UINT lastout;
    PSEG seg;
    std::vector<UCHAR> headbuf;
    PUCHAR stubData;
    FILE*outfile;
    UINT headerSize;
    UINT headerVirtSize;
    UINT stubSize;
    long nameIndex;
    UINT sectionStart;
    UINT headerStart;
    long relocSectNum,importSectNum,exportSectNum,resourceSectNum;
    long importSectLoc = 0, importSectSize = 0;
    long exportSectLoc = 0, exportSectSize = 0;
    long resourceSectLoc = 0, resourceSectSize = 0;
    long relocSectLoc = 0, relocSectSize = 0;
    UINT codeBase=0;
    UINT dataBase=0;
    UINT codeSize=0;
    UINT dataSize=0;

    printf("Generating PE file %s\n",outname.c_str());

    errcount=0;

    /* allocate section entries for imports, exports and relocs if required */
    if(impsreq)
    {
        importSectNum=createOutputSection("imports",
            WINF_INITDATA | WINF_SHARED | WINF_READABLE);
    }
    else
    {
        importSectNum=-1;
    }

    if(expdefs.size())
    {
        exportSectNum=createOutputSection("exports",
            WINF_INITDATA | WINF_SHARED | WINF_READABLE);
    } 
    else
    {
	exportSectNum=-1;
    }

    /* Windows NT requires a reloc section to relocate image files, even */
    /* if it contains no actual fixups */
    relocSectNum=createOutputSection("relocs",
        WINF_INITDATA | WINF_SHARED | WINF_DISCARDABLE | WINF_READABLE);

    if(resource.size())
    {
        resourceSectNum=createOutputSection("resource",
            WINF_INITDATA | WINF_SHARED | WINF_READABLE);
    }
    else
    {
        resourceSectNum=-1;
    }

    /* build header */
    getStub(&stubData,&stubSize);

    headerStart=stubSize; /* get start of PE header */
    headerStart+=7;
    headerStart&=0xfffffff8; /* align PE header to 8 byte boundary */

    headerSize=PE_HEADBUF_SIZE+outlist.size()*PE_OBJECTENTRY_SIZE+stubSize;
    headerVirtSize=headerSize+(objectAlign-1);
    headerVirtSize&=(0xffffffff-(objectAlign-1));
    headerSize+=(fileAlign-1);
    headerSize&=(0xffffffff-(fileAlign-1));


    headbuf.resize(headerSize);

    for(i=0;i<headerSize;i++)
    {
        headbuf[i]=0;
    }

    for(i=0;i<stubSize;i++) /* copy stub file */
        headbuf[i]=stubData[i];

    put32(headbuf, 0x3c, headerStart);         /* store pointer to PE header */
    put32(headbuf, headerStart+PE_SIGNATURE, 0x4550); /* PE\0\0 */
    put16(headbuf, headerStart+PE_MACHINEID, cpuIdent);
    /* store time/date of creation */
    k=(UINT)time(NULL);
    put32(headbuf, headerStart+PE_DATESTAMP, k);
    put16(headbuf, headerStart+PE_HDRSIZE, PE_OPTIONAL_HEADER_SIZE);

    i=PE_FILE_EXECUTABLE | PE_FILE_32BIT;                   /* get flags */
    if(buildDll)
    {
        i|= PE_FILE_LIBRARY;                /* if DLL, flag it */
    }
    put16(headbuf, headerStart+PE_FLAGS, i); /* store them */
    put16(headbuf, headerStart+PE_MAGIC, PE_MAGICNUM); /* store magic number */
    put32(headbuf, headerStart+PE_IMAGEBASE, imageBase); /* store image base */
    put32(headbuf, headerStart+PE_FILEALIGN, fileAlign); /* store image base */
    put32(headbuf, headerStart+PE_OBJECTALIGN, objectAlign); /* store image base */
    headbuf[headerStart+PE_OSMAJOR]=osMajor;
    headbuf[headerStart+PE_OSMINOR]=osMinor;

    headbuf[headerStart+PE_SUBSYSMAJOR]=subsysMajor;
    headbuf[headerStart+PE_SUBSYSMINOR]=subsysMinor;

    put16(headbuf, headerStart+PE_SUBSYSTEM, subSystem);
    put16(headbuf, headerStart+PE_NUMRVAS, PE_NUM_VAS);
    put32(headbuf, headerStart+PE_HEADERSIZE, headerSize);
    put32(headbuf, headerStart+PE_HEAPSIZE, heapSize);
    put32(headbuf, headerStart+PE_HEAPCOMMSIZE, heapCommitSize);
    put32(headbuf, headerStart+PE_STACKSIZE, stackSize);
    put32(headbuf, headerStart+PE_STACKCOMMSIZE, stackCommitSize);

    /* shift segment start addresses up into place and build section headers */
    sectionStart=headerSize;
    j=headerStart+PE_HEADBUF_SIZE;

    for(i=0;i<outlist.size();i++,j+=PE_OBJECTENTRY_SIZE)
    {
        std::string name = outlist[i]->name;
	if(name.size() > 8) name = name.substr(0,8);

        if(name.size())
        {
	    strncpy((char *)&headbuf[j],name.c_str(),name.size());
        }
        k=outlist[i]->virtualSize; /* get virtual size */
	put32(headbuf, j+8, k); /* store virtual size (in memory) of segment */

        if(!padsegments) /* if not padding segments, reduce space consumption */
        {
            for(k=outlist[i]->length-1;(k>=0)&&!GetNbit(outlist[i]->datmask,k);k--);
            k++; /* k=initialised length */
        }
	put32(headbuf, j+16, k); /* store initialised data size */
	put32(headbuf, j+20, sectionStart); /* store physical file offset */

        k+=fileAlign-1;
        k&=(0xffffffff-(fileAlign-1)); /* aligned initialised length */

        sectionStart+=k; /* update section start address for next section */

        outlist[i]->setbase(outlist[i]->getbase()+headerVirtSize+imageBase);
	put32(headbuf, j+12, outlist[i]->getbase()-imageBase);
        k=(outlist[i]->winFlags ^ WINF_NEG_FLAGS) & WINF_IMAGE_FLAGS; /* get characteristice for section */
	put32(headbuf, j+36, k); /* store characteristics */
    }

    put16(headbuf, headerStart+PE_NUMOBJECTS, outlist.size()); /* store number of sections */

    /* build import, export and relocation sections */

    BuildPEImports(importSectNum,&headbuf[headerStart+PE_HEADBUF_SIZE]);
    BuildPEExports(exportSectNum,&headbuf[headerStart+PE_HEADBUF_SIZE],(unsigned char *)outname.c_str());
    BuildPERelocs(relocSectNum,&headbuf[headerStart+PE_HEADBUF_SIZE]);
    BuildPEResources(resourceSectNum,&headbuf[headerStart+PE_HEADBUF_SIZE]);

#if 0
    for(k=headerStart+PE_HEADBUF_SIZE;k<headerSize;)
    {
	int l;
	l = k + 16;
	printf("%08x: ", k);
	for(;k<l&&k<headerSize;k++)
	{
	    printf("%02x ", headbuf[k]&0xff);
	}
	printf(" -- ");
	for(k=l-16;k<l&&k<headerSize;k++)
	{
	    printf("%c", (headbuf[k]>=32) && (headbuf[k]<128) ? headbuf[k] : '.');
	}
	printf("\n");
    }
#endif

    if(errcount)
    {
        exit(1);
    }

    /* get start address */
    if(gotstart)
    {
        GetFixupTarget(&startaddr,&seg,&startaddr.ofs,TRUE);
        if(errcount)
        {
            printf("Invalid start address record\n");
            exit(1);
        }
        i=startaddr.ofs;
        i-=imageBase; /* RVA */
	put32(headbuf, headerStart + PE_ENTRYPOINT, i);
        if(buildDll) /* if library */
        {
            /* flag that entry point should always be called */
	    put16(headbuf, headerStart+PE_DLLFLAGS, 15);
        }
    }
    else
    {
        printf("Warning, no entry point specified\n");
    }

    if(importSectNum>=0) /* if imports, add section entry */
    {
	importSectLoc = outlist[importSectNum]->getbase()-imageBase;
	importSectSize = outlist[importSectNum]->length;
    }
    else 
    {
	for(i = 0; i < seglist.size(); i++)
	{
	    if(seglist[i])
	    {
	        if(seglist[i]->name == ".idata")
	        {
		    importSectLoc = seglist[i]->getbase() - imageBase;
		    importSectSize = seglist[i]->length;
	        }
	    }
	}
    }

    printf("importSectLoc %x importSectSize %x\n", 
	   importSectLoc, importSectSize);

    if(importSectLoc) /* if imports, add section entry */
    {
	put32(headbuf, headerStart + PE_IMPORTRVA, importSectLoc);
	put32(headbuf, headerStart + PE_IMPORTSIZE, importSectSize);
    }

    if(exportSectNum>=0) /* if exports, add section entry */
    {
	exportSectLoc = outlist[exportSectNum]->getbase()-imageBase;
	exportSectSize = outlist[exportSectNum]->length;
    }
    else 
    {
	for(i = 0; i < seglist.size() && seglist[i]; i++)
	{
	    if(seglist[i]->name == ".edata")
	    {
		exportSectLoc = seglist[i]->getbase() - imageBase;
		exportSectSize = seglist[i]->length;
	    }
	}
    }

    printf("exportSectLoc %x exportSectSize %x\n", 
	   exportSectLoc, exportSectSize);

    if(exportSectLoc) 
    {
	put32(headbuf, headerStart+PE_EXPORTRVA, exportSectLoc);
	put32(headbuf, headerStart+PE_EXPORTSIZE, exportSectSize);
    }

    if(relocSectNum>=0) /* if relocs, add section entry */
    {
	relocSectLoc = outlist[relocSectNum]->getbase()-imageBase;
	relocSectSize = outlist[relocSectNum]->length;
    }
    else 
    {
	for(i = 0; i < seglist.size() && seglist[i]; i++)
	{
	    if(seglist[i]->name == "relocs")
	    {
		relocSectLoc = seglist[i]->getbase() - imageBase;
		relocSectSize = seglist[i]->length;
	    }
	}
    }

    printf("relocSectLoc %x relocSectSize %x\n", 
	   relocSectLoc, relocSectSize);

    if(relocSectLoc) /* if relocs, add section entry */
    {
	put32(headbuf, headerStart + PE_FIXUPRVA, relocSectLoc);
	put32(headbuf, headerStart + PE_FIXUPSIZE, relocSectSize);
    }

    if(resourceSectNum>=0) /* if resources, add section entry */
    {
	resourceSectLoc = outlist[resourceSectNum]->getbase()-imageBase;
	resourceSectSize = outlist[resourceSectNum]->length;
    }
    else 
    {
	for(i = 0; i < seglist.size() && seglist[i]; i++)
	{
	    if(seglist[i]->name == ".rsrc")
	    {
		resourceSectLoc = seglist[i]->getbase() - imageBase;
		resourceSectSize = seglist[i]->length;
	    }
	}
    }

    printf("resourceSectLoc %x resourceSectSize %x\n", 
	   resourceSectLoc, resourceSectSize);

    if(resourceSectLoc) /* if relocs, add section entry */
    {
	put32(headbuf, headerStart + PE_RESOURCERVA, resourceSectLoc);
	put32(headbuf, headerStart + PE_RESOURCESIZE, resourceSectSize);
    }

    j=headerStart+PE_HEADBUF_SIZE+(outlist.size()-1)*PE_OBJECTENTRY_SIZE;

    i = get32(headbuf, j+12);
    i+= get32(headbuf, j+8); /* add virtual size of section */

    put32(headbuf, headerStart + PE_IMAGESIZE, i);
    put32(headbuf, headerStart + PE_CODEBASE, codeBase);
    put32(headbuf, headerStart + PE_DATABASE, dataBase);
    put32(headbuf, headerStart + PE_CODESIZE, codeSize);
    put32(headbuf, headerStart + PE_INITDATASIZE, dataSize);

    /* zero out section start for all zero-length segments */
    j=headerStart+PE_HEADBUF_SIZE;
    for(i=0;i<outlist.size();i++,j+=PE_OBJECTENTRY_SIZE)
    {
        /* check if size in file is zero */
        k=headbuf[j+16]|headbuf[j+17]|headbuf[j+18]|headbuf[j+19];
        if(!k)
        {
            /* if so, zero section start */
            headbuf[j+20]=headbuf[j+21]=headbuf[j+22]=headbuf[j+23]=0;
        }
    }

    if(errcount!=0)
    {
        exit(1);
    }

    outfile=fopen(outname.c_str(),"wb");
    if(!outfile)
    {
        printf("Error writing to file %s\n",outname.c_str());
        exit(1);
    }

    for(i=0;i<headerSize;i++)
    {
        fputc(headbuf[i],outfile);
    }

    started=lastout=imageBase+headerVirtSize;

    for(i=0;i<outlist.size();i++)
    {
        if(outlist[i] && ((outlist[i]->attr&SEG_ALIGN) !=SEG_ABS))
        {
            /* ensure section is aligned to file-Align */
            while(ftell(outfile)&(fileAlign-1))
            {
                fputc(0,outfile);
            }
            if(started>outlist[i]->getbase())
            {
                printf("Segment overlap\n");
                printf("Next addr=%08X,base=%08X\n",started,outlist[i]->getbase());
                fclose(outfile);
                exit(1);
            }
            if(padsegments)
            {
                while(started<outlist[i]->getbase())
                {
                    fputc(0,outfile);
                    started++;
                }
            }
            else
            {
                started=outlist[i]->getbase();
            }
            for(j=0;j<outlist[i]->length;j++)
            {
                if(GetNbit(outlist[i]->datmask,j))
                {
                    for(;lastout<started;lastout++)
                    {
                        fputc(0,outfile);
                    }
                    fputc(outlist[i]->data[j],outfile);
                    lastout=started+1;
                }
                else if(padsegments)
                {
                    fputc(0,outfile);
                    lastout=started+1;
                }
                started++;
            }
            started=lastout=outlist[i]->getbase()+outlist[i]->virtualSize;
        }
    }

    fclose(outfile);
}

