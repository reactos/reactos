#include "alink.h"

#define strupr(x)

void loadCoffLib(FILE *libfile,std::string libname)
{
    UINT i,j;
    UINT numsyms;
    UINT modpage;
    UINT memberSize;
    UINT startPoint;
    PUCHAR endptr;
    PLIBFILE p;
    std::string name;
    std::vector<UCHAR> modbuf;
    SORTLIST symlist;
    int x;

    libfiles.push_back(LIBFILE());
    p=&libfiles[libfiles.size()-1];
    p->filename=libname;
    startPoint=ftell(libfile);
    
    if(fread(&buf[0],1,8,libfile)!=8)
    {
	printf("Error reading from file\n");
	exit(1);
    }
    buf[8]=0;
    /* complain if file header is wrong */
    if(strcmp((char *)&buf[0],"!<arch>\n"))
    {
	printf("Invalid library file format - bad file header\n");
	printf("\"%s\"\n",&buf[0]);
	
	exit(1);
    }
    /* read archive member header */
    if(fread(&buf[0],1,60,libfile)!=60)
    {
	printf("Error reading from file\n");
	exit(1);
    }
    if((buf[58]!=0x60) || (buf[59]!='\n'))
    {
	printf("Invalid library file format - bad member signature\n");
	exit(1);
    }
    buf[16]=0;
    /* check name of first linker member */
    if(strcmp((char *)&buf[0],"/               ")) /* 15 spaces */
    {
	printf("Invalid library file format - bad member name\n");
	exit(1);
    }
    buf[58]=0;

    /* strip trailing spaces from size */
    endptr=&buf[57];
    while((endptr>(&buf[48])) && isspace(*endptr))
    {
	*endptr=0;
	endptr--;
    }
    
    /* get size */
    errno=0;
    memberSize=strtoul((char *)&buf[48],(PPCHAR)&endptr,10);
    if(errno || (*endptr))
    {
	printf("Invalid library file format - bad member size\n");
	exit(1);
    }
    if((memberSize<4) && memberSize)
    {
	printf("Invalid library file format - bad member size\n");
	exit(1);
    }
    if(!memberSize)
    {
	numsyms=0;
    }
    else
    {
	if(fread(&buf[0],1,4,libfile)!=4)
	{
	    printf("Error reading from file\n");
	    exit(1);
	}
	numsyms=buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24);
    }
    printf("%u symbols\n",numsyms);
   
    if(numsyms)
    {
	if(fread(&modbuf[0],1,4*numsyms,libfile)!=4*numsyms)
	{
	    printf("Error reading from file\n");
	    exit(1);
	}
    }
    
    for(i=0;i<numsyms;i++)
    {
	modpage=modbuf[3+i*4]+(modbuf[2+i*4]<<8)+(modbuf[1+i*4]<<16)+(modbuf[i*4]<<24);
	    
	for(j=0;TRUE;j++)
	{
	    if((x=getc(libfile))==EOF)
	    {
		printf("Error reading from file\n");
		exit(1);
	    }
	    if(!x) break;
	    name+=(char)x;
	}
	if(!name.size())
	{
	    printf("NULL name for symbol %i\n",i);
	    exit(1);
	}
	if(!case_sensitive)
	{
	    strupr(name);
	}

	std::vector<void *> vv;
	vv.resize(modpage);
	symlist.insert(std::make_pair(name, vv));
    }

    if(numsyms)
    {
	p->symbols=symlist;
    }
    
    /* move to an even byte boundary in the file */
    if(ftell(libfile)&1)
    {
	fseek(libfile,1,SEEK_CUR);
    }

    if(ftell(libfile)!=(startPoint+68+memberSize))
    {
	printf("Invalid first linker member\n");
	printf("Pos=%08X, should be %08X\n",ftell(libfile),startPoint+68+memberSize);
	
	exit(1);
    }

    printf("Loaded first linker member\n");
    
    startPoint=ftell(libfile);

    /* read archive member header */
    if(fread(&buf[0],1,60,libfile)!=60)
    {
	printf("Error reading from file\n");
	exit(1);
    }
    if((buf[58]!=0x60) || (buf[59]!='\n'))
    {
	printf("Invalid library file format - bad member signature\n");
	exit(1);
    }
    buf[16]=0;
    /* check name of second linker member */
    if(!strcmp((char *)&buf[0],"/               ")) /* 15 spaces */
    {
	/* OK, so we've found it, now skip over */
	buf[58]=0;

	/* strip trailing spaces from size */
	endptr=&buf[57];
	while((endptr>(&buf[48])) && isspace(*endptr))
	{
	    *endptr=0;
	    endptr--;
	}
    
	/* get size */
	errno=0;
	memberSize=strtoul((char *)&buf[48],(PPCHAR)&endptr,10);
	if(errno || (*endptr))
	{
	    printf("Invalid library file format - bad member size\n");
	    exit(1);
	}
	if((memberSize<8) && memberSize)
	{
	    printf("Invalid library file format - bad member size\n");
	    exit(1);
	}

	/* move over second linker member */
	fseek(libfile,startPoint+60+memberSize,SEEK_SET);
    
	/* move to an even byte boundary in the file */
	if(ftell(libfile)&1)
	{
	    fseek(libfile,1,SEEK_CUR);
	}
    }
    else
    {
	fseek(libfile,startPoint,SEEK_SET);
    }
    
    
    startPoint=ftell(libfile);
    p->longnames.resize(0);

    /* read archive member header */
    if(fread(&buf[0],1,60,libfile)!=60)
    {
	printf("Error reading from file\n");
	exit(1);
    }
    if((buf[58]!=0x60) || (buf[59]!='\n'))
    {
	printf("Invalid library file format - bad 3rd member signature\n");
	exit(1);
    }
    buf[16]=0;
    /* check name of long names linker member */
    if(!strcmp((char *)&buf[0],"//              ")) /* 14 spaces */
    {
	buf[58]=0;

	/* strip trailing spaces from size */
	endptr=&buf[57];
	while((endptr>(&buf[48])) && isspace(*endptr))
	{
	    *endptr=0;
	    endptr--;
	}
    
	/* get size */
	errno=0;
	memberSize=strtoul((char *)&buf[48],(PPCHAR)&endptr,10);
	if(errno || (*endptr))
	{
	    printf("Invalid library file format - bad member size\n");
	    exit(1);
	}
	if(memberSize)
	{
	    p->longnames.resize(memberSize);
	    if(fread(&p->longnames[0],1,memberSize,libfile)!=memberSize)
	    {
		printf("Error reading from file\n");
		exit(1);
	    }
	}
    }
    else
    {
	/* if no long names member, move back to member header */
	fseek(libfile,startPoint,SEEK_SET);
    }
    

    p->modsloaded=0;
    p->modlist.resize(numsyms);
    p->libtype='C';
    p->blocksize=1;
    p->flags=LIBF_CASESENSITIVE;
}

void loadcofflibmod(PLIBFILE p,FILE *libfile)
{
    char *name;
    UINT ofs;
    
    if(fread(&buf[0],1,60,libfile)!=60)
    {
	printf("Error reading from file\n");
	exit(1);
    }
    if((buf[58]!=0x60) || (buf[59]!='\n'))
    {
	printf("Invalid library member header\n");
	exit(1);
    }
    buf[16]=0;
    if(buf[0]=='/')
    {
	ofs=15;
	while(isspace(buf[ofs]))
	{
	    buf[ofs]=0;
	    ofs--;
	}
	
	ofs=strtoul((char *)&buf[1],&name,10);
	if(!buf[1] || *name)
	{
	    printf("Invalid string number \n");
	    exit(1);
	}
	name=(char *)&p->longnames[0]+ofs;
    }
    else
    {
	name=(char *)&buf[0];
    }
    
    printf("Loading module %s\n",name);
    loadcoff(libfile);
}
