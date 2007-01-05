#include "alink.h"

#define strupr(x)

char t_thred[4];
char f_thred[4];
int t_thredindex[4];
int f_thredindex[4];

void DestroyLIDATA(PDATABLOCK p)
{
    long i;
    if(p->blocks)
    {
	for(i=0;i<p->blocks;i++)
	{
	    DestroyLIDATA(((PPDATABLOCK)(&p->data[0]))[i]);
	}
    }
    delete p;
}

PDATABLOCK BuildLiData(long *bufofs)
{
    PDATABLOCK p;
    long i,j;

    p=new DATABLOCK();
    i=*bufofs;
    p->dataofs=i-lidata->dataofs;
    p->count=buf[i]+256*buf[i+1];
    i+=2;
    if(rectype==LIDATA32)
    {
	p->count+=(buf[i]+256*buf[i+1])<<16;
	i+=2;
    }
    p->blocks=buf[i]+256*buf[i+1];
    i+=2;
    if(p->blocks)
    {
	p->data.resize(p->blocks*sizeof(PDATABLOCK));
	for(j=0;j<p->blocks;j++)
	{
	    ((PPDATABLOCK)&p->data[0])[j]=BuildLiData(&i);
	}
    }
    else
    {
	p->data.resize(buf[i]+1);
	p->data[0]=buf[i];
	i++;
	for(j=0;j<p->data[0];j++,i++)
	{
	    p->data[j+1]=buf[i];
	}
    }
    *bufofs=i;
    return p;
}

void EmitLiData(PDATABLOCK p,long segnum,long *ofs)
{
    long i,j;

    for(i=0;i<p->count;i++)
    {
	if(p->blocks)
	{
	    for(j=0;j<p->blocks;j++)
	    {
		EmitLiData(((PPDATABLOCK)&p->data[0])[j],segnum,ofs);
	    }
	}
	else
	{
	    for(j=0;j<p->data[0];j++,(*ofs)++)
	    {
		if((*ofs)>=seglist[segnum]->length)
		{
		    ReportError(ERR_INV_DATA);
		}
		if(GetNbit(seglist[segnum]->datmask,*ofs))
		{
		    if(seglist[segnum]->data[*ofs]!=p->data[j+1])
		    {
			ReportError(ERR_OVERWRITE);
		    }
		}
		seglist[segnum]->data[*ofs]=p->data[j+1];
		SetNbit(seglist[segnum]->datmask,*ofs);
	    }
	}
    }
}

void RelocLIDATA(PDATABLOCK p,long *ofs,PRELOC r)
{
    long i,j;

    for(i=0;i<p->count;i++)
    {
	if(p->blocks)
	{
	    for(j=0;j<p->blocks;j++)
	    {
		RelocLIDATA(((PPDATABLOCK)&p->data[0])[j],ofs,r);
	    }
	}
	else
	{
	    j=r->ofs-p->dataofs;
	    if(j>=0)
	    {
		if((j<5) || ((li_le==PREV_LI32) && (j<7)))
		{
		    ReportError(ERR_BAD_FIXUP);
		}
		PRELOC rel = new RELOC(*r);
		rel->ofs = *ofs + j;
		relocs.push_back(rel);
		*ofs+=p->data[0];
	    }
	}
    }
}

segframe_t intToFrame(int f)
{
    return (segframe_t)f;
}

segdisp_t intToDisp(int d)
{
    return (segdisp_t)(d + 0x80);
}

void LoadFIXUP(PRELOC r,PUCHAR buf,long *p)
{
    long j;
    int thrednum;

    j=*p;

    // arty -- come back to this
    r->ftype=intToFrame(buf[j]>>4);
    r->ttype=intToDisp(buf[j]&0xf);
    r->disp=0;
    j++;
    if(r->ftype&FIX_THRED)
    {
	thrednum=r->ftype&THRED_MASK;
	if(thrednum>3)
	{
	    ReportError(ERR_BAD_FIXUP);
	}
	r->ftype=intToFrame((f_thred[thrednum]>>2)&7);
	switch(r->ftype)
	{
	case REL_SEGFRAME:
	case REL_GRPFRAME:
	case REL_EXTFRAME:
	    r->frame=f_thredindex[thrednum];
	    if(!r->frame)
	    {
		ReportError(ERR_BAD_FIXUP);
	    }
	    break;
	case REL_LILEFRAME:
	case REL_TARGETFRAME:
	    break;
	default:
	    ReportError(ERR_BAD_FIXUP);
	}
	switch(r->ftype)
	{
	case REL_SEGFRAME:
	    r->frame+=segmin-1;
	    break;
	case REL_GRPFRAME:
	    r->frame+=grpmin-1;
	    break;
	case REL_EXTFRAME:
	    r->frame+=extmin-1;
	    break;
	case REL_LILEFRAME:
	    r->frame=prevseg;
	    break;
	default:
	    break;
	}
    }
    else
    {
	switch(r->ftype)
	{
	case REL_SEGFRAME:
	case REL_GRPFRAME:
	case REL_EXTFRAME:
	    r->frame=GetIndex(buf,&j);
	    if(!r->frame)
	    {
		ReportError(ERR_BAD_FIXUP);
	    }
	    break;
	case REL_LILEFRAME:
	case REL_TARGETFRAME:
	    break;
	default:
	    ReportError(ERR_BAD_FIXUP);
	}
	switch(r->ftype)
	{
	case REL_SEGFRAME:
	    r->frame+=segmin-1;
	    break;
	case REL_GRPFRAME:
	    r->frame+=grpmin-1;
	    break;
	case REL_EXTFRAME:
	    r->frame+=extmin-1;
	    break;
	case REL_LILEFRAME:
	    r->frame=prevseg;
	    break;
	default:
	    break;
	}
    }
    if(r->ttype&FIX_THRED)
    {
	thrednum=r->ttype&3;
	if((r->ttype&4)==0) /* P bit not set? */
	{
	    r->ttype=intToDisp((t_thred[thrednum]>>2)&3); /* DISP present */
	}
	else
	{
	    r->ttype=intToDisp(((t_thred[thrednum]>>2)&3) | 4); /* no disp */
	}
	r->target=t_thredindex[thrednum];
	switch(r->ttype)
	{
	case REL_SEGDISP:
	case REL_GRPDISP:
	case REL_EXTDISP:
	case REL_SEGONLY:
	case REL_GRPONLY:
	case REL_EXTONLY:
	    if(!r->target)
	    {
		ReportError(ERR_BAD_FIXUP);
	    }
	    break;
	case REL_EXPFRAME:
	    break;
	default:
	    ReportError(ERR_BAD_FIXUP);
	}
	switch(r->ttype)
	{
	case REL_SEGDISP:
	    r->target+=segmin-1;
	    break;
	case REL_GRPDISP:
	    r->target+=grpmin-1;
	    break;
	case REL_EXTDISP:
	    r->target+=extmin-1;
	    break;
	case REL_EXPFRAME:
	    break;
	case REL_SEGONLY:
	    r->target+=segmin-1;
	    break;
	case REL_GRPONLY:
	    r->target+=grpmin-1;
	    break;
	case REL_EXTONLY:
	    r->target+=extmin-1;
	    break;
	}
    }
    else
    {
	r->target=GetIndex(buf,&j);
	switch(r->ttype)
	{
	case REL_SEGDISP:
	case REL_GRPDISP:
	case REL_EXTDISP:
	case REL_SEGONLY:
	case REL_GRPONLY:
	case REL_EXTONLY:
	    if(!r->target)
	    {
		ReportError(ERR_BAD_FIXUP);
	    }
	    break;
	case REL_EXPFRAME:
	    break;
	default:
	    ReportError(ERR_BAD_FIXUP);
	}
	switch(r->ttype)
	{
	case REL_SEGDISP:
	    r->target+=segmin-1;
	    break;
	case REL_GRPDISP:
	    r->target+=grpmin-1;
	    break;
	case REL_EXTDISP:
	    r->target+=extmin-1;
	    break;
	case REL_EXPFRAME:
	    break;
	case REL_SEGONLY:
	    r->target+=segmin-1;
	    break;
	case REL_GRPONLY:
	    r->target+=grpmin-1;
	    break;
	case REL_EXTONLY:
	    r->target+=extmin-1;
	    break;
	}
    }
    switch(r->ttype)
    {
    case REL_SEGDISP:
    case REL_GRPDISP:
    case REL_EXTDISP:
    case REL_EXPFRAME:
	r->disp=get16(buf,j);
	j+=2;
	if(rectype==FIXUPP32)
	{
	    r->disp+=get16(buf,j)<<16;
	    j+=2;
	}
	break;
    default:
	break;
    }
    if((r->ftype==REL_TARGETFRAME)&&((r->ttype&FIX_THRED)==0))
    {
	switch(r->ttype)
	{
	case REL_SEGDISP:
	    r->ftype = REL_SEGFRAME;
	    break;
	case REL_GRPDISP:
	    r->ftype = REL_GRPFRAME;
	    break;
	case REL_EXTDISP:
	    r->ftype = REL_EXTFRAME;
	    break;
	case REL_EXPFRAME:
	    r->ftype = REL_EXPFFRAME;
	    break;
	case REL_SEGONLY:
	    r->ftype = REL_SEGFRAME;
	    break;
	case REL_GRPONLY:
	    r->ftype = REL_GRPFRAME;
	    break;
	case REL_EXTONLY:
	    r->ftype = REL_EXTFRAME;
	    break;
	}
	r->frame=r->target;
    }

    *p=j;
}

long loadmod(FILE *objfile)
{
    long modpos;
    long done;
    long i,j,k;
    long segnum,grpnum;
    PRELOC r;
    PPUBLIC pubdef;
    std::string name,aliasName;
    SORTLIST::iterator listnode;
    std::vector<std::string> nlist;

    modpos=0;
    done=0;
    li_le=0;
    lidata=0;

    while(!done)
    {
	if(fread(&buf[0],1,3,objfile)!=3)
	{
	    ReportError(ERR_NO_MODEND);
	}
	rectype=buf[0];
	reclength=buf[1]+256*buf[2];
	if(fread(&buf[0],1,reclength,afile)!=reclength)
	{
	    ReportError(ERR_NO_RECDATA);
	}
	reclength--; /* remove checksum */
	if((!modpos)&&(rectype!=THEADR)&&(rectype!=LHEADR))
	{
	    ReportError(ERR_NO_HEADER);
	}
	switch(rectype)
	{
	case THEADR:
	case LHEADR:
	    if(modpos)
	    {
		ReportError(ERR_EXTRA_HEADER);
	    }
	    modname.push_back(std::string((char *)&buf[1],buf[0]));
	    strupr(modname[nummods]);
	    /*	    printf("Loading module %s\n",modname[nummods]);*/
	    if((buf[0]+1)!=reclength)
	    {
		ReportError(ERR_EXTRA_DATA);
	    }
	    segmin=seglist.size();
	    extmin=externs.size();
	    fixmin=relocs.size();
	    grpmin=grplist.size();
	    impmin=impdefs.size();
	    expmin=expdefs.size();
	    commin=comdefs.size();
	    break;
	case COMENT:
	    li_le=0;
	    if(lidata)
	    {
		DestroyLIDATA(lidata);
		lidata=0;
	    }
	    if(reclength>=2)
	    {
		switch(buf[1])
		{
		case COMENT_LIB_SPEC:
		case COMENT_DEFLIB: {
		    std::string newfilename((char *)&buf[2],reclength-2);
		    if(newfilename.find('.') == std::string::npos)
			newfilename += ".lib";
		    filename.push_back(newfilename);
		} break;
		case COMENT_OMFEXT:
		    if(reclength<4)
		    {
			ReportError(ERR_INVALID_COMENT);
		    }
		    switch(buf[2])
		    {
		    case EXT_IMPDEF: {
			j=4;
			if(reclength<(j+4))
			{
			    ReportError(ERR_INVALID_COMENT);
			}
			IMPREC imp;
			imp.flags=buf[3];
			imp.int_name=std::string((char *)&buf[j+1]);
			if(!case_sensitive)
			{
			    strupr(impdefs[impdefs.size()].int_name);
			}
			j += buf.size() + 1;
			imp.mod_name=std::string((char *)&buf[j+1]);
			j += buf.size() + 1;
			if(!case_sensitive)
			{
			    strupr(impdefs[impdefs.size()].mod_name);
			}
			if(imp.flags)
			{
			    imp.ordinal=buf[j]+256*buf[j+1];
			    j+=2;
			}
			else
			{
			    if(buf[j])
			    {
				imp.imp_name=std::string((char *)&buf[j]);
				j += imp.imp_name.size() + 1;
			    }
			    else
			    {
				imp.imp_name=imp.int_name;
			    }
			}
			impdefs.push_back(imp);
		    } break;
		    case EXT_EXPDEF: {
			EXPREC exp;
			j=4;
			exp.flags=buf[3];
			exp.pubdef=NULL;
			exp.exp_name = std::string((char *)&buf[j]+1,buf[j]);
			j+=buf[j]+1;
			if(buf[j])
			{
			    exp.int_name=std::string((char *)buf[j]+1,buf[j]);
			    if(!case_sensitive)
			    {
				strupr(exp.int_name);
			    }
			}
			else
			{
			    exp.int_name=exp.exp_name;
			}
			j+=buf[j]+1;
			if(exp.flags&EXP_ORD)
			{
			    exp.ordinal=buf[j]+256*buf[j+1];
			}
			else
			{
			    exp.ordinal=0;
			}
			expdefs.push_back(exp);
		    } break;
		    default:
			ReportError(ERR_INVALID_COMENT);
		    }
		    break;
		case COMENT_DOSSEG:
		    break;
		case COMENT_TRANSLATOR:
		case COMENT_INTEL_COPYRIGHT:
		case COMENT_MSDOS_VER:
		case COMENT_MEMMODEL:
		case COMENT_NEWOMF:
		case COMENT_LINKPASS:
		case COMENT_LIBMOD:
		case COMENT_EXESTR:
		case COMENT_INCERR:
		case COMENT_NOPAD:
		case COMENT_WKEXT:
		case COMENT_LZEXT:
		case COMENT_PHARLAP:
		case COMENT_IBM386:
		case COMENT_RECORDER:
		case COMENT_COMMENT:
		case COMENT_COMPILER:
		case COMENT_DATE:
		case COMENT_TIME:
		case COMENT_USER:
		case COMENT_DEPFILE:
		case COMENT_COMMANDLINE:
		case COMENT_PUBTYPE:
		case COMENT_COMPARAM:
		case COMENT_TYPDEF:
		case COMENT_STRUCTMEM:
		case COMENT_OPENSCOPE:
		case COMENT_LOCAL:
		case COMENT_ENDSCOPE:
		case COMENT_SOURCEFILE:
		    break;
		default:
		    printf("COMENT Record (unknown type %02X)\n",buf[1]);
		    break;
		}
	    }
	    break;
	case LLNAMES:
	case LNAMES:
	    j=0;
	    while(j<reclength)
	    {
		int namelen = buf[j]+1;
		nlist.push_back(std::string((char *)&buf[j+1],namelen));
	    }
	    break;
	case SEGDEF:
	case SEGDEF32: {
	    PSEG seg = new SEG();
	    seg->attr=buf[0];
	    j=1;
	    if((seg->attr & SEG_ALIGN)==SEG_ABS)
	    {
		seg->absframe=get16(buf,j);
		seg->absofs=buf[j+2];
		j+=3;
	    }
	    seg->length=buf[j]+256*buf[j+1];
	    j+=2;
	    if(rectype==SEGDEF32)
	    {
		seg->length+=get16(buf,j)<<16;
		j+=2;
	    }
	    if(seg->attr&SEG_BIG)
	    {
		if(rectype==SEGDEF)
		{
		    seg->length+=65536;
		}
		else
		{
		    if((seg->attr&SEG_ALIGN)!=SEG_ABS)
		    {
			ReportError(ERR_SEG_TOO_LARGE);
		    }
		}
	    }
	    seg->name=nlist[GetIndex(buf,&j)-1];
	    seg->classindex=GetIndex(buf,&j)-1;
	    seg->overlayindex=GetIndex(buf,&j)-1;
	    seg->orderindex=-1;
	    if((seg->attr&SEG_ALIGN)!=SEG_ABS)
	    {
		seg->data.resize(seg->length);
		seg->datmask.resize((seg->length+7)/8);
		for(i=0;i<(seg->length+7)/8;i++)
		{
		    seg->datmask[i]=0;
		}
	    }
	    else
	    {
		seg->data.resize(0);
		seg->datmask.resize(0);
		seg->attr&=(0xffff-SEG_COMBINE);
		seg->attr|=SEG_PRIVATE;
	    }
	    switch(seg->attr&SEG_COMBINE)
	    {
	    case SEG_PRIVATE:
	    case SEG_PUBLIC:
	    case SEG_PUBLIC2:
	    case SEG_COMMON:
	    case SEG_PUBLIC3:
		break;
	    case SEG_STACK:
		/* stack segs are always byte aligned */
		seg->attr&=(0xffff-SEG_ALIGN);
		seg->attr|=SEG_BYTE;
		break;
	    default:
		ReportError(ERR_BAD_SEGDEF);
		break;
	    }
	    if((seg->attr&SEG_ALIGN)==SEG_BADALIGN)
	    {
		ReportError(ERR_BAD_SEGDEF);
	    }
	    if((seg->classindex>=0) &&
	       (nlist[seg->classindex] == "CODE" ||
		nlist[seg->classindex] == "TEXT"))
            {
                /* code segment */
                seg->winFlags=WINF_CODE | WINF_INITDATA | WINF_EXECUTE | WINF_READABLE | WINF_NEG_FLAGS;
            }
            else    /* data segment */
                seg->winFlags=WINF_INITDATA | WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;

	    if(seg->name == "$$SYMBOLS" ||
	       seg->name == "$$TYPES")
	    {
		seg->winFlags |=WINF_REMOVE;
	    }
	    seglist.push_back(seg);
	} break;
	case LEDATA:
	case LEDATA32:
	    j=0;
	    prevseg=GetIndex(buf,&j)-1;
	    if(prevseg<0)
	    {
		ReportError(ERR_INV_SEG);
	    }
	    prevseg+=segmin;
	    if((seglist[prevseg]->attr&SEG_ALIGN)==SEG_ABS)
	    {
		ReportError(ERR_ABS_SEG);
	    }
	    prevofs=buf[j]+(buf[j+1]<<8);
	    j+=2;
	    if(rectype==LEDATA32)
	    {
		prevofs+=(buf[j]+(buf[j+1]<<8))<<16;
		j+=2;
	    }
	    for(k=0;j<reclength;j++,k++)
	    {
		if((prevofs+k)>=seglist[prevseg]->length)
		{
		    ReportError(ERR_INV_DATA);
		}
		if(GetNbit(seglist[prevseg]->datmask,prevofs+k))
		{
		    if(seglist[prevseg]->data[prevofs+k]!=buf[j])
		    {
			printf("%08lX: %08lX: %i, %li,%li,%li\n",prevofs+k,j,GetNbit(seglist[prevseg]->datmask,prevofs+k),seglist.size(),segmin,prevseg);
			ReportError(ERR_OVERWRITE);
		    }
		}
		seglist[prevseg]->data[prevofs+k]=buf[j];
		SetNbit(seglist[prevseg]->datmask,prevofs+k);
	    }
	    li_le=PREV_LE;
	    break;
	case LIDATA:
	case LIDATA32:
	    if(lidata)
	    {
		DestroyLIDATA(lidata);
	    }
	    j=0;
	    prevseg=GetIndex(buf,&j)-1;
	    if(prevseg<0)
	    {
		ReportError(ERR_INV_SEG);
	    }
	    prevseg+=segmin;
	    if((seglist[prevseg]->attr&SEG_ALIGN)==SEG_ABS)
	    {
		ReportError(ERR_ABS_SEG);
	    }
	    prevofs=buf[j]+(buf[j+1]<<8);
	    j+=2;
	    if(rectype==LIDATA32)
	    {
		prevofs+=(buf[j]+(buf[j+1]<<8))<<16;
		j+=2;
	    }
	    lidata = new DATABLOCK();
	    lidata->data.resize(sizeof(PDATABLOCK)*(1024/sizeof(DATABLOCK)+1));
	    lidata->blocks=0;
	    lidata->dataofs=j;
	    for(i=0;j<reclength;i++)
	    {
		((PPDATABLOCK)&lidata->data[0])[i]=BuildLiData(&j);
	    }
	    lidata->blocks=i;
	    lidata->count=1;

	    k=prevofs;
	    EmitLiData(lidata,prevseg,&k);
	    li_le=(rectype==LIDATA)?PREV_LI:PREV_LI32;
	    break;
	case LPUBDEF:
	case LPUBDEF32:
	case PUBDEF:
	case PUBDEF32:
	    j=0;
	    grpnum=GetIndex(buf,&j)-1;
	    if(grpnum>=0)
	    {
		grpnum+=grpmin;
	    }
	    segnum=GetIndex(buf,&j)-1;
	    if(segnum<0)
	    {
		j+=2;
	    }
	    else
	    {
		segnum+=segmin;
	    }
	    for(;j<reclength;)
	    {
		pubdef=new PUBLIC();
		pubdef->aliasName="";
		pubdef->grpnum=grpnum;
		pubdef->seg=seglist[segnum];
		name=std::string((char *)&buf[j]+1,buf[j]);
		j+=name.size()+1;
		if(!case_sensitive)
		{
		    strupr(name);
		}
		pubdef->ofs=buf[j]+256*buf[j+1];
		j+=2;
		if((rectype==PUBDEF32) || (rectype==LPUBDEF32))
		{
		    pubdef->ofs+=(buf[j]+256*buf[j+1])<<16;
		    j+=2;
		}
		pubdef->typenum=GetIndex(buf,&j);
		if(rectype==LPUBDEF || rectype==LPUBDEF32)
		{
		    pubdef->modnum=modname.size()-1;
		}
		else
		{
		    pubdef->modnum=0;
		}
		if((listnode=publics.find(name)) != publics.end())
		{
		    for(i=0;i<listnode->second.size();i++)
		    {
			if(((PPUBLIC)listnode->second[i])->modnum==pubdef->modnum)
			{
			    if(((PPUBLIC)listnode->second[i])->aliasName == "")
			    {
				printf("Duplicate public symbol %s\n",name.c_str());
				exit(1);
			    }
			    (*((PPUBLIC)listnode->second[i]))=(*pubdef);
			    pubdef=NULL;
			    break;
			}
		    }
		}
		if(pubdef)
		{
		    SORTLIST::iterator l = publics.find(name);
		    if(l == publics.end()) {
			std::vector<void *> vv;
			vv.push_back((void *)pubdef);
			publics.insert(std::make_pair(name,vv));
		    } else {
			l->second.push_back((void *)pubdef);
		    }
		}
	    }
	    break;
	case LEXTDEF:
	case LEXTDEF32:
	case EXTDEF: {
	    for(j=0;j<reclength;)
	    {
		EXTREC ext;
		ext.name = std::string((char *)&buf[j+1]);
		k=buf[j];
		j+=ext.name.size()+1;
		if(!case_sensitive)
		{
		    strupr(ext.name);
		}
		ext.typenum=GetIndex(buf,&j);
		ext.pubdef=NULL;
		ext.flags=EXT_NOMATCH;
		if((rectype==LEXTDEF) || (rectype==LEXTDEF32))
		{
		    ext.modnum=modname.size()-1;
		}
		else
		{
		    ext.modnum=0;
		}
		externs.push_back(ext);
	    }
	} break;
	case GRPDEF: {
	    PGRP grp = new GRP();
	    j=0;
	    int nindex = GetIndex(buf,&j)-1;
	    if(nindex < 0)
	    {
		ReportError(ERR_BAD_GRPDEF);
	    }
	    grp->name=nlist[GetIndex(buf,&j)-1];
	    grp->numsegs=0;
	    while(j<reclength)
	    {
		if(buf[j]==0xff)
		{
		    j++;
		    i=GetIndex(buf,&j)-1+segmin;
		    if(i<segmin)
		    {
			ReportError(ERR_BAD_GRPDEF);
		    }
		    grp->segindex[grp->numsegs]=seglist[i];
		    grp->numsegs++;
		}
		else
		{
		    ReportError(ERR_BAD_GRPDEF);
		}
	    }
	    grplist.push_back(grp);
	} break;
	case FIXUPP:
	case FIXUPP32:
	    j=0;
	    while(j<reclength)
	    {
		if(buf[j]&0x80)
		{
				/* FIXUP subrecord */
		    if(!li_le)
		    {
			ReportError(ERR_BAD_FIXUP);
		    }
		    r=new RELOC();
		    r->rtype=(buf[j]>>2);
		    r->ofs=buf[j]*256+buf[j+1];
		    j+=2;
		    r->ofs&=0x3ff;
		    r->rtype^=FIX_SELFREL;
		    r->rtype&=FIX_MASK;
		    switch(r->rtype)
		    {
		    case FIX_LBYTE:
		    case FIX_OFS16:
		    case FIX_BASE:
		    case FIX_PTR1616:
		    case FIX_HBYTE:
		    case FIX_OFS16_2:
		    case FIX_OFS32:
		    case FIX_PTR1632:
		    case FIX_OFS32_2:
		    case FIX_SELF_LBYTE:
		    case FIX_SELF_OFS16:
		    case FIX_SELF_OFS16_2:
		    case FIX_SELF_OFS32:
		    case FIX_SELF_OFS32_2:
			break;
		    default:
			ReportError(ERR_BAD_FIXUP);
		    }
		    LoadFIXUP(r,&buf[0],&j);

		    if(li_le==PREV_LE)
		    {
			r->ofs+=prevofs;
			r->seg=seglist[prevseg];
			PRELOC rel = new RELOC(*r);
			relocs.push_back(rel);
		    }
		    else
		    {
			r->seg=seglist[prevseg];
			i=prevofs;
			RelocLIDATA(lidata,&i,r);
			free(r);
		    }
		}
		else
		{
				/* THRED subrecord */
		    i=buf[j]; /* get thred number */
		    j++;
		    if(i&0x40) /* Frame? */
		    {
			f_thred[i&3]=i;
			/* get index if required */
			if((i&0x1c)<0xc)
			{
			    f_thredindex[i&3]=GetIndex(buf,&j);
			}
			i&=3;
		    }
		    else
		    {
			t_thred[i&3]=i;
			/* target always has index */
			t_thredindex[i&3]=GetIndex(buf,&j);
		    }
		}
	    }
	    break;
	case BAKPAT:
	case BAKPAT32:
	    j=0;
	    if(j<reclength) i=GetIndex(buf,&j);
	    i+=segmin-1;
	    if(j<reclength)
	    {
		k=buf[j];
		j++;
	    }
	    while(j<reclength)
	    {
		PRELOC rel = new RELOC();
		switch(k)
		{
		case 0: rel->rtype=FIX_SELF_LBYTE; break;
		case 1: rel->rtype=FIX_SELF_OFS16; break;
		case 2: rel->rtype=FIX_SELF_OFS32; break;
		default:
		    printf("Bad BAKPAT record\n");
		    exit(1);
		}
		rel->ofs=buf[j]+256*buf[j+1];
		j+=2;
		if(rectype==BAKPAT32)
		{
		    rel->ofs+=(buf[j]+256*buf[j+1])<<16;
		    j+=2;
		}
		rel->seg=seglist[i];
		rel->target=i;
		rel->frame=i;
		rel->ttype=REL_SEGDISP;
		rel->ftype=REL_SEGFRAME;
		rel->disp=buf[j]+256*buf[j+1];
		j+=2;
		if(rectype==BAKPAT32)
		{
		    rel->disp+=(buf[j]+256*buf[j+1])<<16;
		    j+=2;
		}
		rel->disp+=rel->ofs;
		switch(k)
		{
		case 0: rel->disp++; break;
		case 1: rel->disp+=2; break;
		case 2: rel->disp+=4; break;
		default:
		    printf("Bad BAKPAT record\n");
		    exit(1);
		}
		relocs.push_back(rel);
	    }
	    break;
	case LINNUM:
	case LINNUM32:
	    printf("LINNUM record\n");
	    break;
	case MODEND:
	case MODEND32:
	    done=1;
	    if(buf[0]&0x40)
	    {
		if(gotstart)
		{
		    ReportError(ERR_MULTIPLE_STARTS);
		}
		gotstart=1;
		j=1;
		LoadFIXUP(&startaddr,&buf[0],&j);
		if(startaddr.ftype==REL_LILEFRAME)
		{
		    ReportError(ERR_BAD_FIXUP);
		}
	    }
	    break;
	case COMDEF:
	    for(j=0;j<reclength;)
	    {
		EXTREC ext;
		ext.name = std::string((char *)&buf[j]);
		k=buf[j];
		j+=ext.name.size()+1;
		if(!case_sensitive)
		{
		    strupr(ext.name);
		}
		ext.typenum=GetIndex(buf,&j);
		ext.pubdef=NULL;
		ext.flags=EXT_NOMATCH;
		ext.modnum=0;
		if(buf[j]==0x61)
		{
		    j++;
		    i=buf[j];
		    j++;
		    if(i==0x81)
		    {
			i=buf[j]+256*buf[j+1];
			j+=2;
		    }
		    else if(i==0x84)
		    {
			i=buf[j]+256*buf[j+1]+65536*buf[j+2];
			j+=3;
		    }
		    else if(i==0x88)
		    {
			i=buf[j]+256*buf[j+1]+65536*buf[j+2]+(buf[j+3]<<24);
			j+=4;
		    }
		    k=i;
		    i=buf[j];
		    j++;
		    if(i==0x81)
		    {
			i=buf[j]+256*buf[j+1];
			j+=2;
		    }
		    else if(i==0x84)
		    {
			i=buf[j]+256*buf[j+1]+65536*buf[j+2];
			j+=3;
		    }
		    else if(i==0x88)
		    {
			i=buf[j]+256*buf[j+1]+65536*buf[j+2]+(buf[j+3]<<24);
			j+=4;
		    }
		    i*=k;
		    k=1;
		}
		else if(buf[j]==0x62)
		{
		    j++;
		    i=buf[j];
		    j++;
		    if(i==0x81)
		    {
			i=buf[j]+256*buf[j+1];
			j+=2;
		    }
		    else if(i==0x84)
		    {
			i=buf[j]+256*buf[j+1]+65536*buf[j+2];
			j+=3;
		    }
		    else if(i==0x88)
		    {
			i=buf[j]+256*buf[j+1]+65536*buf[j+2]+(buf[j+3]<<24);
			j+=4;
		    }
		    k=0;
		}
		else
		{
		    printf("Unknown COMDEF data type %02X\n",buf[j]);
		    exit(1);
		}
		PCOMREC com = new COMREC();
		com->length=i;
		com->isFar=k;
		com->modnum=0;
		com->name=ext.name;
		externs.push_back(ext);
		comdefs.push_back(com);
	    }

	    break;
	case COMDAT:
	case COMDAT32:
	    printf("COMDAT section\n");
	    exit(1);
	    
	    break;
	case ALIAS:
	    printf("ALIAS record\n");
	    j=0;
	    name=std::string((char *)&buf[j]+1,buf[j]);
	    j+=name.size()+1;
	    if(!case_sensitive)
	    {
		strupr(name);
	    }
	    printf("ALIAS name:%s\n",name.c_str());
	    aliasName=std::string((char *)&buf[j]+1,buf[j]);
	    j+=aliasName.size()+1;
	    if(!case_sensitive)
	    {
		strupr(aliasName);
	    }
	    printf("Substitute name:%s\n",aliasName.c_str());
	    if(name.size())
	    {
		printf("Cannot use alias a blank name\n");
		exit(1);
	    }
	    if(!aliasName.size())
	    {
		printf("No Alias name specified for %s\n",name.c_str());
		exit(1);
	    }
	    pubdef=new PUBLIC();
	    pubdef->seg=0;
	    pubdef->grpnum=-1;
	    pubdef->typenum=-1;
	    pubdef->ofs=0;
	    pubdef->modnum=0;
	    pubdef->aliasName=aliasName;
	    if((listnode=publics.find(name)) != publics.end())
	    {
		for(i=0;i<listnode->second.size();i++)
		{
		    if(((PPUBLIC)listnode->second[i])->modnum==pubdef->modnum)
		    {
			if(((PPUBLIC)listnode->second[i])->aliasName != "")
			{
			    printf("Warning, two aliases for %s, using %s\n",name.c_str(),((PPUBLIC)listnode->second[i])->aliasName.c_str());
			}
			free(pubdef);
			pubdef=NULL;
			break;
		    }
		}
	    }
	    if(pubdef)
	    {
		SORTLIST::iterator l = publics.find(name);
		if(l == publics.end()) {
		    std::vector<void *> vv;
		    vv.push_back((void *)pubdef);
		    publics.insert(std::make_pair(name,vv));
		} else {
		    l->second.push_back((void *)pubdef);
		}
	    }
	    break;
	default:
	    ReportError(ERR_UNKNOWN_RECTYPE);
	}
	filepos+=4+reclength;
	modpos+=4+reclength;
    }
    if(lidata)
    {
	DestroyLIDATA(lidata);
    }
    return 0;
}

void loadlib(FILE *libfile,std::string libname)
{
    unsigned int i,j,k,n;
    std::string name;
    unsigned short modpage;
    PLIBFILE p;
    SORTLIST symlist;

    libfiles.push_back(LIBFILE());
    p=&libfiles[libfiles.size()-1];
    
    p->filename=libname;

    if(fread(&buf[0],1,3,libfile)!=3)
    {
	printf("Error reading from file\n");
	exit(1);
    }
    p->blocksize=buf[1]+256*buf[2];
    if(fread(&buf[0],1,p->blocksize,libfile)<1)
    {
	printf("Error reading from file\n");
	exit(1);
    }
    p->blocksize+=3;
    p->dicstart=buf[0]+(buf[1]<<8)+(buf[2]<<16)+(buf[3]<<24);
    p->numdicpages=buf[4]+256*buf[5];
    p->flags=buf[6];
    p->libtype='O';
	
    fseek(libfile,p->dicstart,SEEK_SET);

    for(i=0;i<p->numdicpages;i++)
    {
	if(fread(&buf[0],1,512,libfile)!=512)
	{
	    printf("Error reading from file\n");
	    exit(1);
	}
	for(j=0;j<37;j++)
	{
	    k=buf[j]*2;
	    if(k)
	    {
		name=std::string((char *)&buf[k+1],buf[k]);
		k+=buf[k]+1;
		modpage=buf[k]+256*buf[k+1];
		if(!(p->flags&LIBF_CASESENSITIVE) || !case_sensitive)
		{
		    strupr(name);
		}
		if(name[name.size()-1]=='!')
		{
		    name = "";
		}
		else
		{
		    std::vector<void *> items;
		    items.resize(modpage);
		    symlist.insert(std::make_pair(name,items));
		}
	    }
	}
    }

    p->symbols=symlist;
    p->modsloaded=0;
}

void loadlibmod(UINT libnum,UINT modpage)
{
    PLIBFILE p;
    FILE *libfile;
    UINT i;

    p=&libfiles[libnum];

    /* don't open a module we've loaded already */
    for(i=0;i<p->modsloaded;i++)
    {
	if(p->modlist[i]==modpage) return;
    }

    libfile=fopen(p->filename.c_str(),"rb");
    if(!libfile)
    {
	printf("Error opening file %s\n",p->filename.c_str());
	exit(1);
    }
    fseek(libfile,modpage*p->blocksize,SEEK_SET);
    switch(p->libtype)
    {
    case 'O':
	loadmod(libfile);
	break;
    case 'C':
	loadcofflibmod(p,libfile);
	break;
    default:
	printf("Unknown library file format\n");
	exit(1);
    }
	
    p->modlist[p->modsloaded]=modpage;
    p->modsloaded++;
    fclose(libfile);
}

void loadres(FILE *f)
{
    unsigned char buf[32];
    static unsigned char buf2[32]={0,0,0,0,0x20,0,0,0,0xff,0xff,0,0,0xff,0xff,0,0,
				   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    UINT i,j;
    UINT hdrsize,datsize;
    std::vector<unsigned char> data;
    std::vector<unsigned char> hdr;

    if(fread(buf,1,32,f)!=32)
    {
	printf("Invalid resource file\n");
	exit(1);
    }
    if(memcmp(buf,buf2,32))
    {
	printf("Invalid resource file\n");
        exit(1);
    }
    printf("Loading Win32 Resource File\n");
    while(!feof(f))
    {
	i=ftell(f);
	if(i&3)
	{
	    fseek(f,4-(i&3),SEEK_CUR);
	}
        i=fread(buf,1,8,f);
        if(i==0 && feof(f)) return;
        if(i!=8)
	{
	    printf("Invalid resource file, no header\n");
            exit(1);
	}
	datsize=buf[0]+(buf[1]<<8)+(buf[2]<<16)+(buf[3]<<24);
        hdrsize=buf[4]+(buf[5]<<8)+(buf[6]<<16)+(buf[7]<<24);
	if(hdrsize<16)
        {
	    printf("Invalid resource file, bad header\n");
            exit(1);
        }
        hdr.resize(hdrsize);
	if(fread(&hdr[0],1,hdrsize-8,f)!=(hdrsize-8))
	{
	    printf("Invalid resource file, missing header\n");
	    exit(1);
	}
	/* if this is a NULL resource, then skip */
	if(!datsize && (hdrsize==32) && !memcmp(buf2+8,&hdr[0],24))
	{
	    continue;
	}
	if(datsize)
	{
	    data.resize(datsize);
	    if(fread(&data[0],1,datsize,f)!=datsize)
	    {
		printf("Invalid resource file, no data\n");
		exit(1);
	    }
	}
	else data.resize(0);
	RESOURCE res;
	res.data=data;
	res.length=datsize;
	i=0;
	hdrsize-=8;
	if((hdr[i]==0xff) && (hdr[i+1]==0xff))
	{
	    res.xtypename.resize(0);
	    res.typeid=hdr[i+2]+256*hdr[i+3];
	    i+=4;
	}
	else
	{
	    for(j=i;(j<(hdrsize-1))&&(hdr[j]|hdr[j+1]);j+=2);
	    if(hdr[j]|hdr[j+1])
	    {
		printf("Invalid resource file, bad name\n");
		exit(1);
	    }
	    res.xtypename.resize(j-i+2);
	    memcpy(&res.xtypename[0],&hdr[i],j-i+2);
	    i=j+5;
	    i&=0xfffffffc;
	}
	if(i>hdrsize)
	{
	    printf("Invalid resource file, overflow\n");
	    exit(1);
	}
	if((hdr[i]==0xff) && (hdr[i+1]==0xff))
	{
	    res.name.resize(0);
	    res.id=hdr[i+2]+256*hdr[i+3];
	    i+=4;
	}
	else
	{
	    for(j=i;(j<(hdrsize-1))&&(hdr[j]|hdr[j+1]);j+=2);
	    if(hdr[j]|hdr[j+1])
	    {
		printf("Invalid resource file,bad name (2)\n");
		exit(1);
	    }
	    res.name.resize(j-i+2);
	    memcpy(&res.name[0],&hdr[i],j-i+2);
	    i=j+5;
	    i&=0xfffffffc;
	}
	i+=6; /* point to Language ID */
	if(i>hdrsize)
	{
	    printf("Invalid resource file, overflow(2)\n");
	    exit(1);
	}
	res.languageid=hdr[i]+256*hdr[i+1];
	resource.push_back(res);
    }
}
