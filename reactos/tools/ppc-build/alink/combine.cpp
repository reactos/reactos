#include "alink.h"

void fixpubsegs(PSEG src,PSEG dest,UINT shift)
{
    UINT i,j;
    PPUBLIC q;
    SORTLIST::iterator it;
    
    for(it=publics.begin();it!=publics.end();++it)
    {
	for(j=0;j<it->second.size();++it)
	{
	    q=(PPUBLIC)it->second[j];
	    if(q->seg==src)
	    {
		q->seg=dest;
		q->ofs+=shift;
	    }
	}
    }
}

void fixpubgrps(int src,int dest)
{
    UINT i,j;
    PPUBLIC q;
    SORTLIST::iterator it;

    for(it=publics.begin();it!=publics.end();++it)
    {
	for(j=0;j<it->second.size();++j)
	{
	    q=(PPUBLIC)it->second[j];
	    if(q->grpnum==src)
	    {
		q->grpnum=dest;
	    }
	}
    }
}

void combine_segments(PSEG destseg,PSEG srcseg)
{
    UINT k,n;
    std::vector<unsigned char> p,q;
    long a1,a2;

    assert(destseg != srcseg);

    k=destseg->length;
    switch(srcseg->attr&SEG_ALIGN)
    {
    case SEG_WORD:
	a2=2;
	k=(k+1)&0xfffffffe;
	break;
    case SEG_PARA:
	a2=16;
	k=(k+0xf)&0xfffffff0;
	break;
    case SEG_PAGE:
	a2=0x100;
	k=(k+0xff)&0xffffff00;
	break;
    case SEG_DWORD:
	a2=4;
	k=(k+3)&0xfffffffc;
	break;
    case SEG_MEMPAGE:
	a2=0x1000;
	k=(k+0xfff)&0xfffff000;
	break;
    case SEG_8BYTE:
	a2=8;
	k=(k+7)&0xfffffff8;
	break;
    case SEG_32BYTE:
	a2=32;
	k=(k+31)&0xffffffe0;
	break;
    case SEG_64BYTE:
	a2=64;
	k=(k+63)&0xffffffc0;
	break;
    default:
	a2=1;
	break;
    }
    switch(destseg->attr&SEG_ALIGN)
    {
    case SEG_WORD:
	a1=2;
	break;
    case SEG_DWORD:
	a1=4;
	break;
    case SEG_8BYTE:
	a1=8;
	break;
    case SEG_PARA:
	a1=16;
	break;
    case SEG_32BYTE:
	a1=32;
	break;
    case SEG_64BYTE:
	a1=64;
	break;
    case SEG_PAGE:
	a1=0x100;
	break;
    case SEG_MEMPAGE:
	a1=0x1000;
	break;
    default:
	a1=1;
	break;
    }

    srcseg->setbase(k);
    p.resize(k+srcseg->length);
    q.resize((k+srcseg->length+7)/8);
    for(k=0;k<destseg->length;k++)
    {
	if(GetNbit(destseg->datmask,k))
	{
	    SetNbit(q,k);
	    p[k]=destseg->data[k];
	}
	else
	{
	    ClearNbit(q,k);
	}
    }
    for(;k<srcseg->getbase();k++)
    {
	ClearNbit(q,k);
    }
    for(;k<(srcseg->getbase()+srcseg->length);k++)
    {
	if(GetNbit(srcseg->datmask,k-srcseg->getbase()))
	{
	    p[k]=srcseg->data[k-srcseg->getbase()];
	    SetNbit(q,k);
	}
	else
	{
	    ClearNbit(q,k);
	}
    }
    destseg->length=k;
    if(a2>a1) destseg->attr=srcseg->attr;
    destseg->winFlags |= srcseg->winFlags;
    destseg->data=p;
    destseg->datmask=q;

    fixpubsegs(srcseg,destseg,srcseg->getbase());

    for(k=0;k<relocs.size();k++)
    {
	if(relocs[k]->seg==srcseg)
	{
	    relocs[k]->seg=destseg;
	    relocs[k]->ofs+=srcseg->getbase();
	}
	if(relocs[k]->ttype==REL_SEGDISP)
	{
	    if(relocs[k]->targseg==srcseg)
	    {
		relocs[k]->targseg=destseg;
		relocs[k]->disp=srcseg->getbase();
	    }
	}
	else if(relocs[k]->ttype==REL_SEGONLY)
	{
	    if(relocs[k]->targseg==srcseg)
	    {
		relocs[k]->targseg=destseg;
		relocs[k]->ttype=REL_SEGDISP;
		relocs[k]->disp=srcseg->getbase();
	    }
	}
	if((relocs[k]->ftype==REL_SEGFRAME) ||
	   (relocs[k]->ftype==REL_LILEFRAME))
	{
	    if(relocs[k]->seg==srcseg)
	    {
		relocs[k]->seg=destseg;
	    }
	}
    }

    if(gotstart)
    {
	if(startaddr.ttype==REL_SEGDISP)
	{
	    if(startaddr.targseg==srcseg)
	    {
		startaddr.targseg=destseg;
		startaddr.disp+=srcseg->getbase();
	    }
	}
	else if(startaddr.ttype==REL_SEGONLY)
	{
	    if(startaddr.targseg==srcseg)
	    {
		startaddr.targseg=destseg;
		startaddr.disp=srcseg->getbase();
		startaddr.ttype=REL_SEGDISP;
	    }
	}
	if((startaddr.ftype==REL_SEGFRAME) ||
	   (startaddr.ftype==REL_LILEFRAME))
	{
	    if(startaddr.seg==srcseg)
	    {
		startaddr.seg=destseg;
	    }
	}
    }

    for(k=0;k<grplist.size();k++)
    {
	if(grplist[k])
	{
	    for(n=0;n<grplist[k]->numsegs;n++)
	    {
		if(grplist[k]->segindex[n]==srcseg)
		{
		    grplist[k]->segindex[n]=destseg;
		}
	    }
	}
    }

    srcseg=0;
}

void combine_common(long i,long j)
{
    UINT k,n;
    std::vector<unsigned char> p,q;

    if(seglist[j]->length>seglist[i]->length)
    {
	k=seglist[i]->length;
	seglist[i]->length=seglist[j]->length;
	seglist[j]->length=k;
	p=seglist[i]->data;
	q=seglist[i]->datmask;
	seglist[i]->data=seglist[j]->data;
	seglist[i]->datmask=seglist[j]->datmask;
    }
    else
    {
	p=seglist[j]->data;
	q=seglist[j]->datmask;
    }
    for(k=0;k<seglist[j]->length;k++)
    {
	if(GetNbit(q,k))
	{
	    if(GetNbit(seglist[i]->datmask,k))
	    {
		if(seglist[i]->data[k]!=p[k])
		{
		    ReportError(ERR_OVERWRITE);
		}
	    }
	    else
	    {
		SetNbit(seglist[i]->datmask,k);
		seglist[i]->data[k]=p[k];
	    }
	}
    }

    fixpubsegs(seglist[j],seglist[i],0);
    
    for(k=0;k<relocs.size();k++)
    {
	if(relocs[k]->seg==seglist[j])
	{
	    relocs[k]->seg=seglist[i];
	}
	if(relocs[k]->ttype==REL_SEGDISP)
	{
	    if(relocs[k]->target==j)
	    {
		relocs[k]->target=i;
	    }
	}
	else if(relocs[k]->ttype==REL_SEGONLY)
	{
	    if(relocs[k]->target==j)
	    {
		relocs[k]->target=i;
	    }
	}
	if((relocs[k]->ftype==REL_SEGFRAME) ||
	   (relocs[k]->ftype==REL_LILEFRAME))
	{
	    if(relocs[k]->frame==j)
	    {
		relocs[k]->frame=i;
	    }
	}
    }

    if(gotstart)
    {
	if(startaddr.ttype==REL_SEGDISP)
	{
	    if(startaddr.target==j)
	    {
		startaddr.target=i;
	    }
	}
	else if(startaddr.ttype==REL_SEGONLY)
	{
	    if(startaddr.target==j)
	    {
		startaddr.target=i;
	    }
	}
	if((startaddr.ftype==REL_SEGFRAME) ||
	   (startaddr.ftype==REL_LILEFRAME))
	{
	    if(startaddr.frame==j)
	    {
		startaddr.frame=i;
	    }
	}
    }

    for(k=0;k<grplist.size();k++)
    {
	if(grplist[k])
	{
	    for(n=0;n<grplist[k]->numsegs;n++)
	    {
		if(grplist[k]->segindex[n]==seglist[j])
		{
		    grplist[k]->segindex[n]=seglist[i];
		}
	    }
	}
    }

    free(seglist[j]);
    seglist[j]=0;
}

void combine_groups(long i,long j)
{
    long n,m;
    char match;

    for(n=0;n<grplist[j]->numsegs;n++)
    {
	match=0;
	for(m=0;m<grplist[i]->numsegs;m++)
	{
	    if(grplist[j]->segindex[n]==grplist[i]->segindex[m])
	    {
		match=1;
	    }
	}
	if(!match)
	{
	    grplist[i]->numsegs++;
	    grplist[i]->segindex[grplist[i]->numsegs]=grplist[j]->segindex[n];
	}
    }
    free(grplist[j]);
    grplist[j]=0;

    fixpubgrps(j,i);

    for(n=0;n<relocs.size();n++)
    {
	if(relocs[n]->ftype==REL_GRPFRAME)
	{
	    if(relocs[n]->frame==j)
	    {
		relocs[n]->frame=i;
	    }
	}
	if((relocs[n]->ttype==REL_GRPONLY) || (relocs[n]->ttype==REL_GRPDISP))
	{
	    if(relocs[n]->target==j)
	    {
		relocs[n]->target=i;
	    }
	}
    }

    if(gotstart)
    {
	if((startaddr.ttype==REL_GRPDISP) || (startaddr.ttype==REL_GRPONLY))
	{
	    if(startaddr.target==j)
	    {
		startaddr.target=i;
	    }
	}
	if(startaddr.ftype==REL_GRPFRAME)
	{
	    if(startaddr.frame==j)
	    {
		startaddr.frame=i;
	    }
	}
    }
}

void combineBlocks()
{
    long i,j,k;
    std::string name;
    long attr;
    UINT count;
    std::vector<UINT> slist;
    UINT curseg;

    for(i=0;i<seglist.size();i++)
    {
	if(seglist[i]&&((seglist[i]->attr&SEG_ALIGN)!=SEG_ABS))
	{
	    if(seglist[i]->winFlags & WINF_COMDAT) continue; /* don't combine COMDAT segments */
	    name=seglist[i]->name;
	    attr=seglist[i]->attr&(SEG_COMBINE|SEG_USE32);

	    switch(attr&SEG_COMBINE)
	    {
	    case SEG_STACK:
		for(j=i+1;j<seglist.size();j++)
		{
		    if(!seglist[j]) continue;
		    if(seglist[j]->winFlags & WINF_COMDAT) continue;
		    if((seglist[j]->attr&SEG_ALIGN)==SEG_ABS) continue;
		    if((seglist[j]->attr&SEG_COMBINE)!=SEG_STACK) continue;
		    combine_segments(seglist[i],seglist[j]);
		}
		break;
	    case SEG_PUBLIC:
	    case SEG_PUBLIC2:
	    case SEG_PUBLIC3:
		slist.resize(1);
		slist[0] = i;
		/* get list of segments to combine */
		for(j=i+1,count=1;j<seglist.size();j++)
		{
		    if(!seglist[j]) continue;
		    if(seglist[j]->winFlags & WINF_COMDAT) continue;
		    if((seglist[j]->attr&SEG_ALIGN)==SEG_ABS) continue;
		    if(attr!=(seglist[j]->attr&(SEG_COMBINE|SEG_USE32))) continue;
		    if(name != seglist[j]->name) continue;
		    slist.push_back(j);
		}
		/* sort them by sortorder */
		for(j=0;j<count;j++)
		{
		    for(k=j+1;k<count;k++)
		    {
			if(seglist[slist[k]]->orderindex<0 ||
			   seglist[slist[j]]->orderindex<0) continue;

			if(seglist[slist[j]]->name >
			   seglist[slist[k]]->name) {
			    int x = slist[k];
			    slist[k] = slist[j];
			    slist[j] = x;
			}
		    }
		}
		/* then combine in that order */
		for(j=1;j<count;j++)
		{
		    combine_segments(seglist[slist[0]],seglist[slist[j]]);
		}
		break;
	    case SEG_COMMON:
		for(j=i+1;j<seglist.size();j++)
		{
		    if((seglist[j]&&((seglist[j]->attr&SEG_ALIGN)!=SEG_ABS)) &&
		       ((seglist[i]->attr&(SEG_ALIGN|SEG_COMBINE|SEG_USE32))==(seglist[j]->attr&(SEG_ALIGN|SEG_COMBINE|SEG_USE32)))
		       &&
		       (name == seglist[j]->name)
		       && !(seglist[j]->winFlags & WINF_COMDAT)
		       )
		    {
			combine_common(i,j);
		    }
		}
		break;
	    default:
		break;
	    }
	}
    }

    for(i=0;i<grplist.size();i++)
    {
	if(grplist[i])
	{
	    for(j=i+1;j<grplist.size();j++)
	    {
		if(!grplist[j]) continue;
		if(grplist[i]->name == grplist[j]->name)
		{
		    combine_groups(i,j);
		}
	    }
	}
    }
}


