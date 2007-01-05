#include "alink.h"

#define strupr(x)

char case_sensitive=1;
char padsegments=0;
char mapfile=0;
std::string mapname;
unsigned short maxalloc=0xffff;
int output_type=OUTPUT_EXE;
std::string outname;

FILE *afile=0;
UINT filepos=0;
long reclength=0;
unsigned char rectype=0;
char li_le=0;
UINT prevofs=0;
long prevseg=0;
long gotstart=0;
RELOC startaddr;
UINT imageBase=0;
UINT fileAlign=1;
UINT objectAlign=1;
UINT stackSize;
UINT stackCommitSize;
UINT heapSize;
UINT heapCommitSize;
unsigned char osMajor,osMinor;
unsigned char subsysMajor,subsysMinor;
unsigned int subSystem;
int buildDll=FALSE;
std::string stubName;

long errcount=0;

std::vector<unsigned char> buf;
PDATABLOCK lidata;

std::vector<std::string> namelist;
std::vector<PSEG> seglist;
std::vector<PSEG> outlist;
std::vector<PGRP> grplist;
SORTLIST publics;
std::vector<EXTREC> externs;
std::vector<PCOMREC> comdefs;
std::vector<PRELOC> relocs;
std::vector<IMPREC> impdefs;
std::vector<EXPREC> expdefs;
std::vector<LIBFILE> libfiles;
std::vector<RESOURCE> resource;
SORTLIST comdats;
std::vector<std::string> modname;
std::vector<std::string> filename;
UINT namemin=0,
pubmin=0,
    segmin=0,
    grpmin=0,
    extmin=0,
    commin=0,
    fixmin=0,
    impmin=0,impsreq=0,
    expmin=0;
UINT libPathCount=0;
std::vector<std::string> libPath;
std::string entryPoint;
int cpuIdent = 0;

void processArgs(int argc,char **argv)
{
    long i,j;
    int helpRequested=FALSE;
    UINT setbase,setfalign,setoalign;
    UINT setstack,setstackcommit,setheap,setheapcommit;
    int setsubsysmajor,setsubsysminor,setosmajor,setosminor;
    unsigned char setsubsys;
    int gotbase=FALSE,gotfalign=FALSE,gotoalign=FALSE,gotsubsys=FALSE;
    int gotstack=FALSE,gotstackcommit=FALSE,gotheap=FALSE,gotheapcommit=FALSE;
    int gotsubsysver=FALSE,gotosver=FALSE;
    char *p,*q;
    int c;
    std::vector<std::string> newargs;
    FILE *argFile;

    for(i=1;i<argc;i++)
	newargs.push_back(argv[i]);

    for(i=0;i<newargs.size();i++)
    {
	/* cater for response files */
	if(newargs[i][0]=='@')
	{
	    argFile=fopen(&newargs[i][1],"rt");
	    if(!argFile)
	    {
		printf("Unable to open response file \"%s\"\n",&newargs[i][1]);
		exit(1);
	    }
	    for(j=0;j<newargs.size();j++)
	    {
		newargs.push_back(newargs[j]);
	    }
	    p=NULL;
	    j=0;
	    while((c=fgetc(argFile))!=EOF)
	    {
		if(c==';') /* allow comments, starting with ; */
		{
		    while(((c=fgetc(argFile))!=EOF) && (c!='\n')); /* loop until end of line */
		    /* continue main loop */
		    continue;
		}
		if(isspace(c))
		{
		    if(p) /* if we've got an argument, add to list */
		    {
			newargs.push_back(p);
			/* clear pointer and length indicator */
			p=NULL;
			j=0;
		    }
		    /* and continue */
		    continue;
		}
		if(c=='"')
		{
		    /* quoted strings */
		    while(((c=fgetc(argFile))!=EOF) && (c!='"')) /* loop until end of string */
		    {
			if(c=='\\')
			{
			    c=fgetc(argFile);
			    if(c==EOF)
			    {
				printf("Missing character to escape in quoted string, unexpected end of file found\n");
				exit(1);
			    }
			}
			    
			p+=c;
			j++;
		    }
		    if(c==EOF)
		    {
			printf("Unexpected end of file encountered in quoted string\n");
			exit(1);
		    }
		    
		    /* continue main loop */
		    continue;
		}
		/* if no special case, then add to string */
		p+=c;
	    }
	    if(p)
	    {
		newargs.push_back(p);
	    }
	    fclose(argFile);
	    newargs=newargs;
	}
	else if(newargs[i][0]==SWITCHCHAR)
	{
	    if(newargs[i].size()<2)
	    {
		printf("Invalid argument \"%s\"\n",newargs[i].c_str());
		exit(1);
	    }
	    switch(newargs[i][1])
	    {
	    case 'c':
		if(newargs[i].size()==2)
		{
		    case_sensitive=1;
		    break;
		}
		else if(newargs[i].size()==3)
		{
		    if(newargs[i][2]=='+')
		    {
			case_sensitive=1;
			break;
		    }
		    else if(newargs[i][2]=='-')
		    {
			case_sensitive=0;
			break;
		    }
		}
		printf("Invalid switch %s\n",newargs[i].size());
		exit(1);
		break;
	    case 'p':
		switch(newargs[i].size())
		{
		case 2:
		    padsegments=1;
		    break;
		case 3:
		    if(newargs[i][2]=='+')
		    {
			padsegments=1;
			break;
		    }
		    else if(newargs[i][2]=='-')
		    {
			padsegments=0;
			break;
		    }
		default:
		    printf("Invalid switch %s\n",newargs[i].c_str());
		    exit(1);
		}
		break;
	    case 'm':
		switch(newargs[i].size())
		{
		case 2:
		    mapfile=1;
		    break;
		case 3:
		    if(newargs[i][2]=='+')
		    {
			mapfile=1;
			break;
		    }
		    else if(newargs[i][2]=='-')
		    {
			mapfile=0;
			break;
		    }
		default:
		    printf("Invalid switch %s\n",newargs[i].c_str());
		    exit(1);
		}
		break;
	    case 'o':
		switch(newargs[i].size())
		{
		case 2:
		    if(i<(newargs.size()-1))
		    {
			i++;
			if(!outname.size())
			{
			    outname = newargs[i];
			}
			else
			{
			    printf("Can't specify two output names\n");
			    exit(1);
			}
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		default:
		    if(!strcmp(&newargs[i][2],"EXE"))
		    {
			output_type=OUTPUT_EXE;
			imageBase=0;
			fileAlign=1;
			objectAlign=1;
			stackSize=0;
			stackCommitSize=0;
			heapSize=0;
			heapCommitSize=0;
		    }
		    else if(!strcmp(&newargs[i][2],"COM"))
		    {
			output_type=OUTPUT_COM;
			imageBase=0;
			fileAlign=1;
			objectAlign=1;
			stackSize=0;
			stackCommitSize=0;
			heapSize=0;
			heapCommitSize=0;
		    }
		    else if(!strcmp(&newargs[i][2],"PE"))
		    {
			output_type=OUTPUT_PE;
			imageBase=WIN32_DEFAULT_BASE;
			fileAlign=WIN32_DEFAULT_FILEALIGN;
			objectAlign=WIN32_DEFAULT_OBJECTALIGN;
			stackSize=WIN32_DEFAULT_STACKSIZE;
			stackCommitSize=WIN32_DEFAULT_STACKCOMMITSIZE;
			heapSize=WIN32_DEFAULT_HEAPSIZE;
			heapCommitSize=WIN32_DEFAULT_HEAPCOMMITSIZE;
			subSystem=WIN32_DEFAULT_SUBSYS;
			subsysMajor=WIN32_DEFAULT_SUBSYSMAJOR;
			subsysMinor=WIN32_DEFAULT_SUBSYSMINOR;
			osMajor=WIN32_DEFAULT_OSMAJOR;
			osMinor=WIN32_DEFAULT_OSMINOR;
		    }
		    else if(!strcmp(&newargs[i][1],"objectalign"))
		    {
			if(i<(newargs.size()-1))
			{
			    i++;
			    setoalign=strtoul(&newargs[i][0],&p,0);
			    if(p[0]) /* if not at end of arg */
			    {
				printf("Bad object alignment\n");
				exit(1);
			    }
			    if((setoalign<512)|| (setoalign>(256*1048576))
			       || (getBitCount(setoalign)>1))
			    {
				printf("Bad object alignment\n");
				exit(1);
			    }
			    gotoalign=TRUE;
			}
			else
			{
			    printf("Invalid switch %s\n",newargs[i].c_str());
			    exit(1);
			}
		    }
		    else if(!strcmp(&newargs[i][1],"osver"))
		    {
			if(i<(newargs.size()-1))
			{
			    i++;
			    if(sscanf(&newargs[i][0],"%d.%d%n",&setosmajor,&setosminor,&j)!=2)
			    {
				printf("Invalid version number %s\n",newargs[i].c_str());
				exit(1);
			    }
			    if((j!=newargs[i].size()) || (setosmajor<0) || (setosminor<0)
			       || (setosmajor>65535) || (setosminor>65535))
			    {
				printf("Invalid version number %s\n",newargs[i].c_str());
				exit(1);
			    }
			    gotosver=TRUE;
			}
			else
			{
			    printf("Invalid switch %s\n",newargs[i].c_str());
			    exit(1);
			}
			break;
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		break;
	    case 'L':
		if(newargs[i].size()==2)
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			j=newargs[i].size();
			if(newargs[i][j-1]!=PATH_CHAR)
			{
				/* append a path separator if not present */
			    std::string lp = newargs[i];
			    lp += PATH_CHAR;
			    libPath.push_back(lp);
			}
			else
			{
			    libPath.push_back(newargs[i]);
			}
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		printf("Invalid switch %s\n",newargs[i].c_str());
		exit(1);
		break;
	    case 'h':
	    case 'H':
	    case '?':
		if(newargs[i].size()==2)
		{
		    helpRequested=TRUE;
		}
		else if(!strcmp(&newargs[i][1],"heapsize"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			setheap=strtoul(newargs[i].c_str(),&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad heap size\n");
			    exit(1);
			}
			gotheap=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(&newargs[i][1],"heapcommitsize"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			setheapcommit=strtoul(newargs[i].c_str(),&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad heap commit size\n");
			    exit(1);
			}
			gotheapcommit=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		break;
	    case 'b':
		if(!strcmp(&newargs[i][1],"base"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			setbase=strtoul(newargs[i].c_str(),&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad image base\n");
			    exit(1);
			}
			if(setbase&0xffff)
			{
			    printf("Bad image base\n");
			    exit(1);
			}
			gotbase=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		else
		{
		    printf("Invalid switch %s\n",newargs[i].c_str());
		    exit(1);
		}
		break;
	    case 's':
		if(!strcmp(&newargs[i][1],"subsys"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			if(!strcmp(newargs[i].c_str(),"gui")
			   || !strcmp(newargs[i].c_str(),"windows")
			   || !strcmp(newargs[i].c_str(),"win"))
			{
			    setsubsys=PE_SUBSYS_WINDOWS;
			    gotsubsys=TRUE;
			}
			else if(!strcmp(newargs[i].c_str(),"char")
				|| !strcmp(newargs[i].c_str(),"console")
				|| !strcmp(newargs[i].c_str(),"con"))
			{
			    setsubsys=PE_SUBSYS_CONSOLE;
			    gotsubsys=TRUE;
			}
			else if(!strcmp(newargs[i].c_str(),"native"))
			{
			    setsubsys=PE_SUBSYS_NATIVE;
			    gotsubsys=TRUE;
			}
			else if(!strcmp(newargs[i].c_str(),"posix"))
			{
			    setsubsys=PE_SUBSYS_POSIX;
			    gotsubsys=TRUE;
			}
			else
			{
			    printf("Invalid subsystem id %s\n",newargs[i].c_str());
			    exit(1);
			}
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(&newargs[i][1],"subsysver"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			if(sscanf(newargs[i].c_str(),"%d.%d%n",&setsubsysmajor,&setsubsysminor,&j)!=2)
			{
			    printf("Invalid version number %s\n",newargs[i].c_str());
			    exit(1);
			}
			if((j!=newargs[i].size()) || (setsubsysmajor<0) || (setsubsysminor<0)
			   || (setsubsysmajor>65535) || (setsubsysminor>65535))
			{
			    printf("Invalid version number %s\n",newargs[i].c_str());
			    exit(1);
			}
			gotsubsysver=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(&newargs[i][1],"stacksize"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			setstack=strtoul(newargs[i].c_str(),&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad stack size\n");
			    exit(1);
			}
			gotstack=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(&newargs[i][1],"stackcommitsize"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			setstackcommit=strtoul(newargs[i].c_str(),&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad stack commit size\n");
			    exit(1);
			}
			gotstackcommit=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(&newargs[i][1],"stub"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			stubName=newargs[i];
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		    break;
		}
		else
		{
		    printf("Invalid switch %s\n",newargs[i].c_str());
		    exit(1);
		}
		break;
	    case 'f':
		if(!strcmp(&newargs[i][1],"filealign"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			setfalign=strtoul(newargs[i].c_str(),&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad file alignment\n");
			    exit(1);
			}
			if((setfalign<512)|| (setfalign>65536)
			   || (getBitCount(setfalign)>1))
			{
			    printf("Bad file alignment\n");
			    exit(1);
			}
			gotfalign=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		}
		else
		{
		    printf("Invalid switch %s\n",newargs[i].c_str());
		    exit(1);
		}
		break;
	    case 'd':
		if(!strcmp(&newargs[i][1],"dll"))
		{
		    buildDll=TRUE;
		}
		else
		{
		    printf("Invalid switch %s\n",newargs[i].c_str());
		    exit(1);
		}
		break;
	    case 'e':
		if(!strcmp(&newargs[i][1],"entry"))
		{
		    if(i<(newargs.size()-1))
		    {
			i++;
			entryPoint=newargs[i];
		    }
		    else
		    {
			printf("Invalid switch %s\n",newargs[i].c_str());
			exit(1);
		    }
		}
		else
		{
		    printf("Invalid switch %s\n",newargs[i].c_str());
		    exit(1);
		}
		break;
		
	    default:
		printf("Invalid switch %s\n",newargs[i].c_str());
		exit(1);
	    }
	}
	else
	{
	    std::string newfilename = newargs[i];
	    if(newfilename.find('.') == std::string::npos)
		newfilename += DEFAULT_EXTENSION;
	    filename.push_back(newfilename);
	}
    }
    if(helpRequested || !filename.size())
    {
	printf("Usage: ALINK [file [file [...]]] [options]\n");
	printf("\n");
	printf("    Each file may be an object file, a library, or a Win32 resource\n");
	printf("    file. If no extension is specified, .obj is assumed. Modules are\n");
	printf("    only loaded from library files if they are required to match an\n");
	printf("    external reference.\n");
	printf("    Options and files may be listed in any order, all mixed together.\n");
	printf("\n");
	printf("The following options are permitted:\n");
	printf("\n");
	printf("    @name   Load additional options from response file name\n");
	printf("    -c      Enable Case sensitivity\n");
	printf("    -c+     Enable Case sensitivity\n");
	printf("    -c-     Disable Case sensitivity\n");
	printf("    -p      Enable segment padding\n");
	printf("    -p+     Enable segment padding\n");
	printf("    -p-     Disable segment padding\n");
	printf("    -m      Enable map file\n");
	printf("    -m+     Enable map file\n");
	printf("    -m-     Disable map file\n");
	printf("----Press Enter to continue---");
	while(((c=getchar())!='\n') && (c!=EOF));
	printf("\n");
	printf("    -h      Display this help list\n");
	printf("    -H      \"\n");
	printf("    -?      \"\n");
	printf("    -L ddd  Add directory ddd to search list\n");
	printf("    -o name Choose output file name\n");
	printf("    -oXXX   Choose output format XXX\n");
	printf("        Available options are:\n");
	printf("            COM - MSDOS COM file\n");
	printf("            EXE - MSDOS EXE file\n");
	printf("            PE  - Win32 PE Executable\n");
	printf("    -entry name   Use public symbol name as the entry point\n");
	printf("\nOptions for PE files:\n");
	printf("    -base addr        Set base address of image\n");
	printf("    -filealign addr   Set section alignment in file\n");
	printf("    -objectalign addr Set section alignment in memory\n");
	printf("    -subsys xxx       Set subsystem used\n");
	printf("        Available options are:\n");
	printf("            console   Select character mode\n");
	printf("            con       \"\n");
	printf("            char      \"\n");
	printf("            windows   Select windowing mode\n");
	printf("            win       \"\n");
	printf("            gui       \"\n");
	printf("            native    Select native mode\n");
	printf("            posix     Select POSIX mode\n");
	printf("    -subsysver x.y    Select subsystem version x.y\n");
	printf("    -osver x.y        Select OS version x.y\n");
	printf("    -stub xxx         Use xxx as the MSDOS stub\n");
	printf("    -dll              Build DLL instead of EXE\n");
	printf("    -stacksize xxx    Set stack size to xxx\n");
	printf("    -stackcommitsize xxx Set stack commit size to xxx\n");
	printf("    -heapsize xxx     Set heap size to xxx\n");
	printf("    -heapcommitsize xxx Set heap commit size to xxx\n");
	exit(0);
    }
    if((output_type!=OUTPUT_PE) &&
       (gotoalign || gotfalign || gotbase || gotsubsys || gotstack ||
	gotstackcommit || gotheap || gotheapcommit || buildDll || stubName.size() || 
	gotsubsysver || gotosver))
    {
	printf("Option not supported for non-PE output formats\n");
	exit(1);
    }
    if(gotstack)
    {
	stackSize=setstack;
    }
    if(gotstackcommit)
    {
	stackCommitSize=setstackcommit;
    }
    if(stackCommitSize>stackSize)
    {
	printf("Stack commit size is greater than stack size, committing whole stack\n");
	stackCommitSize=stackSize;
    }
    if(gotheap)
    {
	heapSize=setheap;
    }
    if(gotheapcommit)
    {
	heapCommitSize=setheapcommit;
    }
    if(heapCommitSize>heapSize)
    {
	printf("Heap commit size is greater than heap size, committing whole heap\n");
	heapCommitSize=heapSize;
    }
    if(gotoalign)
    {
	objectAlign=setoalign;
    }
    if(gotfalign)
    {
	fileAlign=setfalign;
    }
    if(gotbase)
    {
	imageBase=setbase;
    }
    if(gotsubsys)
    {
	subSystem=setsubsys;
    }
    if(gotsubsysver)
    {
	subsysMajor=setsubsysmajor;
	subsysMinor=setsubsysminor;
    }
    if(gotosver)
    {
	osMajor=setosmajor;
	osMinor=setosminor;
    }
}

void matchExterns()
{
    long i,j,old_nummods,k;
    int n;
    std::string name;
    SORTLIST::iterator listnode, l;
    PPUBLIC pubdef;

    do
    {
	for(i=0;i<expdefs.size();i++)
	{
	    if(expdefs[i].pubdef) continue;
	    if((listnode=publics.find(expdefs[i].int_name)) != publics.end())
	    {
		for(k=0;k<listnode->second.size();k++)
		{
		    /* exports can only match global publics */
		    if(((PPUBLIC)listnode->second[k])->modnum==0)
		    {
			expdefs[i].pubdef=(PPUBLIC)listnode->second[k];
			break;
		    }
		}
	    }
	}
	for(i=0;i<externs.size();i++)
	{
	    /* skip if we've already matched a public symbol */
	    /* as they override all others */
	    if(externs[i].flags==EXT_MATCHEDPUBLIC) continue;
	    externs[i].flags=EXT_NOMATCH;
	    if((listnode=publics.find(externs[i].name)) != publics.end())
	    {
		for(k=0;k<listnode->second.size();k++)
		{
		    /* local publics can only match externs in same module */
		    /* and global publics can only match global externs */
		    if(((PPUBLIC)listnode->second[k])->modnum==externs[i].modnum)
		    {
			externs[i].pubdef=(PPUBLIC)listnode->second[k];
			externs[i].flags=EXT_MATCHEDPUBLIC;
			break;
		    }
		}
	    }
	    if(externs[i].flags==EXT_NOMATCH)
	    {
		for(j=0;j<impdefs.size();j++)
		{
		    if((externs[i].name == impdefs[j].int_name)
		       || ((case_sensitive==0) &&
			   (externs[i].name == impdefs[j].int_name)))
		    {
			externs[i].flags=EXT_MATCHEDIMPORT;
			externs[i].impnum=j;
			impsreq++;
		    }
		}
	    }
	    if(externs[i].flags==EXT_NOMATCH)
	    {
		for(j=0;j<expdefs.size();j++)
		{
		    if(!expdefs[j].pubdef) continue;
		    if((externs[i].name == expdefs[j].exp_name)
		       || ((case_sensitive==0) &&
			   !(externs[i].name == expdefs[j].exp_name)))
		    {
			externs[i].pubdef=expdefs[j].pubdef;
			externs[i].flags=EXT_MATCHEDPUBLIC;
		    }
		}
	    }
	}

	old_nummods=modname.size();
	for(i=0;(i<expdefs.size())&&(modname.size()==old_nummods);i++)
	{
	    if(!expdefs[i].pubdef)
	    {
		for(k=0;k<libfiles.size();++k)
		{
		    name = expdefs[i].int_name;
		    if(!(libfiles[k].flags&LIBF_CASESENSITIVE))
		    {
			strupr(name);
		    }
		
		    if((listnode=libfiles[k].symbols.find(name)) != libfiles[k].symbols.end())
		    {
			loadlibmod(k,listnode->second.size());
			break;
		    }
		}
	    }
	}
	for(i=0;(i<externs.size())&&(modname.size()==old_nummods);i++)
	{
	    if(externs[i].flags==EXT_NOMATCH)
	    {
		for(k=0;k<libfiles.size();++k)
		{
		    name = externs[i].name;
		    if(!(libfiles[k].flags&LIBF_CASESENSITIVE))
		    {
			strupr(name);
		    }
		
		    if((listnode=libfiles[k].symbols.find(name)) != libfiles[k].symbols.end())
		    {
			loadlibmod(k,listnode->second.size());
			break;
		    }
		}
	    }
	}
	for(l=publics.begin();
	    (l!=publics.end())&&(modname.size()==old_nummods);
	    ++l)
	{
	    for(k=0;k<l->second.size();k++)
	    {
		pubdef=(PPUBLIC)l->second[k];
		if(pubdef->aliasName == "") continue;
		if((listnode=publics.find(pubdef->aliasName)) != publics.end())
		{
		    for(j=0;j<listnode->second.size();j++)
		    {
			if((((PPUBLIC)listnode->second[j])->modnum==pubdef->modnum)
			   && ((PPUBLIC)listnode->second[j])->aliasName == "")
			{
			    /* if we've found a match for the alias, then kill the alias */
			    (*pubdef)=(*((PPUBLIC)listnode->second[j]));
			    break;
			}
		    }
		}
		if(pubdef->aliasName == "") continue;
		for(k=0;k<libfiles.size();++k)
		{
		    name = pubdef->aliasName;
		    if(!(libfiles[k].flags&LIBF_CASESENSITIVE))
		    {
			strupr(name);
		    }
		
		    if((listnode=libfiles[k].symbols.find(name)) != libfiles[k].symbols.end())
		    {
			loadlibmod(k,listnode->second.size());
			break;
		    }
		}
		
	    }
	}
	
    } while (old_nummods!=modname.size());
}

void matchComDefs()
{
    int i,j,k;
    int comseg;
    int comfarseg;
    SORTLIST::iterator listnode;
    PPUBLIC pubdef;

    if(!comdefs.size()) return;

    for(i=0;i<comdefs.size();i++)
    {
	if(!comdefs[i]) continue;
	for(j=0;j<i;j++)
	{
	    if(!comdefs[j]) continue;
	    if(comdefs[i]->modnum!=comdefs[j]->modnum) continue;
	    if(comdefs[i]->name == comdefs[j]->name)
	    {
		if(comdefs[i]->isFar!=comdefs[j]->isFar)
		{
		    printf("Mismatched near/far type for COMDEF %s\n",comdefs[i]->name.c_str());
		    exit(1);
		}
		if(comdefs[i]->length>comdefs[j]->length)
		    comdefs[j]->length=comdefs[i]->length;
		free(comdefs[i]);
		comdefs[i]=0;
		break;
	    }
	}
    }

    for(i=0;i<comdefs.size();i++)
    {
	if(!comdefs[i]) continue;
	if((listnode=publics.find(comdefs[i]->name)) != publics.end())
	{
	    for(j=0;j<listnode->second.size();j++)
	    {
		/* local publics can only match externs in same module */
		/* and global publics can only match global externs */
		if((((PPUBLIC)listnode->second[j])->modnum==comdefs[i]->modnum)
		   && ((PPUBLIC)listnode->second[j])->aliasName == "")
		{
		    free(comdefs[i]);
		    comdefs[i]=0;
		    break;
		}
	    }
	}
    }

    PSEG seg = new SEG();
    seg->name = "COMDEFS";
    comseg=seglist.size();
    seglist.push_back(seg);

    for(i=0;i<grplist.size();i++)
    {
	if(grplist[i]->name == "") continue;
	if(grplist[i]->name != "DGROUP")
	{
	    if(grplist[i]->numsegs==0) continue; /* don't add to an emtpy group */
	    /* because empty groups are special */
	    /* else add to group */
	    grplist[i]->segindex[grplist[i]->numsegs]=seglist[comseg];
	    grplist[i]->numsegs++;
	    break;
	}
    }

    seg = new SEG();
    seg->name = "FARCOMDEFS";
    comfarseg=seglist.size();
    seglist.push_back(seg);

    for(i=0;i<comdefs.size();i++)
    {
	if(!comdefs[i]) continue;
	pubdef=new PUBLIC();
	if(comdefs[i]->isFar)
	{
	    if(comdefs[i]->length>65536)
	    {
		seg = new SEG();
		seg->name = "FARCOMDEFS";
		seg->length=comdefs[i]->length;
		seg->datmask.resize((comdefs[i]->length+7)/8);
		for(j=0;j<(comdefs[i]->length+7)/8;j++)
		    seglist[seglist.size()-1]->datmask[j]=0;
		pubdef->seg=seglist[seglist.size()-1];
		seglist.push_back(seg);
		pubdef->ofs=0;
	    }
	    else if((comdefs[i]->length+seglist[comfarseg]->length)>65536)
	    {
		seglist[comfarseg]->datmask.resize((seglist[comfarseg]->length+7)/8);
		for(j=0;j<(seglist[comfarseg]->length+7)/8;j++)
		    seglist[comfarseg]->datmask[j]=0;

		seg = new SEG();
		seg->name = "FARCOMDEFS";
		seg->length=comdefs[i]->length;
		comfarseg=seglist.size();
		seglist.push_back(seg);
		pubdef->seg=seglist[comfarseg];
		pubdef->ofs=0;
	    }
	    else
	    {
		pubdef->seg=seglist[comfarseg];
		pubdef->ofs=seglist[comfarseg]->length;
		seglist[comfarseg]->length+=comdefs[i]->length;
	    }
	}
	else
	{
	    pubdef->seg=seglist[comseg];
	    pubdef->ofs=seglist[comseg]->length;
	    seglist[comseg]->length+=comdefs[i]->length;
	}
	pubdef->modnum=comdefs[i]->modnum;
	pubdef->grpnum=-1;
	pubdef->typenum=0;
	if((listnode=publics.find(comdefs[i]->name)) != publics.end())
	{
	    for(j=0;j<listnode->second.size();++j)
	    {
		if(((PPUBLIC)listnode->second[j])->modnum==pubdef->modnum)
		{
		    if(((PPUBLIC)listnode->second[j])->aliasName == "")
		    {
			printf("Duplicate public symbol %s\n",comdefs[i]->name.c_str());
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
	    SORTLIST::iterator it = publics.find(comdefs[i]->name);
	    if(it == publics.end()) 
	    {
		std::vector<void *> vv;
		vv.push_back((void *)pubdef);
		publics.insert(std::make_pair(comdefs[i]->name, vv));
	    } else {
		it->second.push_back((void *)pubdef);
	    }
	}
    }
    seglist[comfarseg]->datmask.reserve((seglist[comfarseg]->length+7)/8);
    for(j=0;j<(seglist[comfarseg]->length+7)/8;j++)
	seglist[comfarseg]->datmask[j]=0;


    seglist[comseg]->datmask.reserve((seglist[comseg]->length+7)/8);
    for(j=0;j<(seglist[comseg]->length+7)/8;j++)
	seglist[comseg]->datmask[j]=0;


    for(i=0;i<expdefs.size();i++)
    {
	if(expdefs[i].pubdef) continue;
	if((listnode=publics.find(expdefs[i].int_name)) != publics.end())
	{
	    for(j=0;j<listnode->second.size();j++)
	    {
		/* global publics only can match exports */
		if(((PPUBLIC)listnode->second[j])->modnum==0)
		{
		    expdefs[i].pubdef=(PPUBLIC)listnode->second[j];
		    break;
		}
	    }
	}
    }
    for(i=0;i<externs.size();i++)
    {
	if(externs[i].flags!=EXT_NOMATCH) continue;
	if((listnode=publics.find(externs[i].name)) != publics.end())
	{
	    for(j=0;j<listnode->second.size();j++)
	    {
		/* global publics only can match exports */
		if(((PPUBLIC)(listnode->second[j]))->modnum==externs[i].modnum)
		{
		    externs[i].pubdef=(PPUBLIC)(listnode->second[j]);
		    externs[i].flags=EXT_MATCHEDPUBLIC;
		    break;
		}
	    }
	}
    }
}

void sortSegments()
{
    long i,j;
    PSEG k, baseSeg;
    UINT base=0,align;

    for(i=0;i<seglist.size();i++)
    {
	if(seglist[i])
	{
	    if((seglist[i]->attr&SEG_ALIGN)!=SEG_ABS)
	    {
		seglist[i]->absframe=0;
	    }
	}
    }

    for(i=0;i<grplist.size();i++)
    {
	if(grplist[i])
	{
	    grplist[i]->seg=NULL;
	    for(j=0;j<grplist[i]->numsegs;j++)
	    {
		k=grplist[i]->segindex[j];
		if(!k)
		{
		    printf("Error - group %s contains non-existent segment\n",grplist[i]->name.c_str());
		    exit(1);
		}
		/* don't add removed sections */
		if(k->winFlags & WINF_REMOVE)
		{
		    continue;
		}
		/* add non-absolute segment */
		if((k->attr&SEG_ALIGN)!=SEG_ABS)
		{
		    switch(k->attr&SEG_ALIGN)
		    {
		    case SEG_WORD:
			align=2;
			break;
		    case SEG_DWORD:
			align=4;
			break;
		    case SEG_8BYTE:
			align=0x8;
			break;
		    case SEG_PARA:
			align=0x10;
			break;
		    case SEG_32BYTE:
			align=0x20;
			break;
		    case SEG_64BYTE:
			align=0x40;
			break;
		    case SEG_PAGE:
			align=0x100;
			break;
		    case SEG_MEMPAGE:
			align=0x1000;
			break;
		    case SEG_BYTE:
		    default:
			align=1;
			break;
		    }
		    if(align<objectAlign)
		    {
			align=objectAlign;
		    }
		    base=(base+align-1)&(0xffffffff-(align-1));
		    k->setbase(base);
		    if(k->length>0)
		    {
			base+=k->length;
			if(k->absframe!=0)
			{
			    printf("Error - Segment %s part of more than one group\n",k->name.c_str());
			    exit(1);
			}
			k->absframe=1;
			k->absofs=i+1;
			if(!grplist[i]->seg)
			{
			    grplist[i]->seg=k;
			}
			if(outlist.size()==0)
			{
			    baseSeg=k;
			}
			else
			{
			    outlist[outlist.size()-1]->virtualSize=k->getbase()-
				outlist[outlist.size()-1]->getbase();
			}
			outlist.push_back(k);
		    }
		}
	    }
	}
    }
    for(i=0;i<seglist.size();i++)
    {
	if(seglist[i])
	{
	    /* don't add removed sections */
	    if(seglist[i]->winFlags & WINF_REMOVE)
	    {
		continue;
	    }
	    /* add non-absolute segment, not already dealt with */
	    if(((seglist[i]->attr&SEG_ALIGN)!=SEG_ABS) &&
	       !seglist[i]->absframe)
	    {
		switch(seglist[i]->attr&SEG_ALIGN)
		{
		case SEG_WORD:
		case SEG_BYTE:
		case SEG_DWORD:
		case SEG_PARA:
		    align=0x10;
		    break;
		case SEG_PAGE:
		    align=0x100;
		    break;
		case SEG_MEMPAGE:
		    align=0x1000;
		    break;
		default:
		    align=1;
		    break;
		}
		if(align<objectAlign)
		{
		    align=objectAlign;
		}
		base=(base+align-1)&(0xffffffff-(align-1));
		seglist[i]->setbase(base);
		if(seglist[i]->length>0)
		{
		    base+=seglist[i]->length;
		    seglist[i]->absframe=1;
		    seglist[i]->absofs=0;
		    if(outlist.size()==0)
		    {
			baseSeg=seglist[i];
		    }
		    else
		    {
			outlist[outlist.size()-1]->virtualSize=seglist[i]->getbase()-
			    outlist[outlist.size()-1]->getbase();
		    }
		    outlist.push_back(seglist[i]);
		}
	    }
	    else if((seglist[i]->attr&SEG_ALIGN)==SEG_ABS)
	    {
		seglist[i]->setbase((seglist[i]->absframe<<4)+seglist[i]->absofs);
	    }
	}
    }
    /* build size of last segment in output list */
    if(outlist.size())
    {
	outlist[outlist.size()-1]->virtualSize=
	    (outlist[outlist.size()-1]->length+objectAlign-1)&
	    (0xffffffff-(objectAlign-1));
    }
    for(i=0;i<grplist.size();i++)
    {
	if(grplist[i] && (!grplist[i]->seg)) grplist[i]->seg=baseSeg;
    }
}

void printRelocs(FILE *afile)
{
    int i;

    if(relocs.size())
    {
	INT_TO_NAME disptypes[] = {
	    { REL_SEGDISP, "REL_SEGDISP" },
	    { REL_GRPDISP, "REL_GRPDISP" },
	    { REL_EXTDISP, "REL_EXTDISP" },
	    { REL_EXPFRAME, "REL_EXPFRAME" },
	    { REL_SEGONLY, "REL_SEGONLY" },
	    { REL_GRPONLY, "REL_GRPONLY" },
	    { REL_EXTONLY, "REL_EXTONLY" },
	    { 0 }
	};

	INT_TO_NAME frametypes[] = {
	    { REL_SEGFRAME, "REL_SEGFRAME" },
	    { REL_GRPFRAME, "REL_GRPFRAME" },
	    { REL_EXTFRAME, "REL_EXTFRAME" },
	    { REL_LILEFRAME,"REL_LILEFRAME"},
	    { REL_TARGETFRAME,"REL_TARGETFRAME"},
	    { 0 }
	};

	INT_TO_NAME reloctype[] = {
	    { FIX_LBYTE, "FIX_LBYTE" },
	    { FIX_OFS16, "FIX_OFS16" },
	    { FIX_BASE, "FIX_BASE" },
	    { FIX_PTR1616, "FIX_PTR1616" },
	    { FIX_HBYTE, "FIX_HBYTE" },
	    { FIX_OFS16_2, "FIX_OFS16_2" },
	    { FIX_OFS32, "FIX_OFS32" },
	    { FIX_PTR1632, "FIX_PTR1632" },
	    { FIX_RVA32, "FIX_RVA32" },
	    { FIX_REL32, "FIX_REL32" },
	    { FIX_PPC_ADDR24, "FIX_PPC_ADDR24" },
	    { FIX_PPC_REL24, "FIX_PPC_REL24" },
	    { FIX_PPC_RVA16LO, "FIX_PPC_RVA16LO" },
	    { FIX_PPC_RVA16HA, "FIX_PPC_RVA16HA" },
	    { FIX_PPC_SECTOFF, "FIX_PPC_SECTOFF" },
	    { 0 }
	};

	fprintf(afile,"\n %li relocs:\n",relocs.size());
	for(i=0;i<relocs.size();i++)
	{
	    fprintf(afile,
		    "%-10s(%d) -> %-10s(%d) type %-16s(%d) ofs %08x disp %08x"
		    " | frame (%-10s) target (%-10s)"
		    " | addr %08x off %08x info %08x\n",
		    findName(frametypes, relocs[i]->ftype),
		    relocs[i]->ftype, 
		    findName(disptypes, relocs[i]->ttype),
		    relocs[i]->ttype,
		    findName(reloctype, relocs[i]->rtype),
		    relocs[i]->rtype,
		    relocs[i]->ofs,
		    relocs[i]->disp,
		    (relocs[i]->seg ? 
		     relocs[i]->seg->name.c_str() :
		     externs[relocs[i]->frame].name.c_str()),
		    (relocs[i]->targseg ? 
		     relocs[i]->targseg->name.c_str() :
		     externs[relocs[i]->target].name.c_str()),
		    relocs[i]->orig.r_address,
		    relocs[i]->orig.r_offset,
		    relocs[i]->orig.r_info);
	}
    }
}

void loadFiles()
{
    long i,j,k;
    std::string name;

    for(i=0;i<filename.size();i++)
    {
	afile=fopen(filename[i].c_str(),"rb");
	if(!strchr(filename[i].c_str(),PATH_CHAR))
	{
	    /* if no path specified, search library path list */
	    for(j=0;!afile && j<libPathCount;j++)
	    {
		name = libPath[j] + filename[i];
		afile=fopen(name.c_str(),"rb");
		if(afile)
		{
		    filename[i]=name;
		    name="";
		}
		else
		{
		    name="";
		}
	    }
	}
	if(!afile)
	{
	    printf("Error opening file %s\n",filename[i].c_str());
	    exit(1);
	}
	for(k=0;k<i;++k)
	{
	    if(filename[i] == filename[k]) break;
	}
	if(k!=i)
	{
	    fclose(afile);
	    continue;
	}
	
	filepos=0;
	printf("Loading file %s\n",filename[i].c_str());
	j=fgetc(afile);
	fseek(afile,0,SEEK_SET);
	switch(j)
	{
	case 0x7f:
	    modname.push_back(filename[i]);
	    loadelf(afile);
	    break;
	case LIBHDR:
	    fseek(afile,1,SEEK_SET);
	    j = fgetc(afile);
	    fseek(afile,0,SEEK_SET);

	    if(j == 1)
		loadcoff(afile);
	    else
		loadlib(afile,filename[i]);
	    //loadmod(afile);
	    //loadcoff(afile);
	    break;
	case THEADR:
	case LHEADR:
	    loadmod(afile);
	    break;
	case 0:
	    loadres(afile);
	    break;
	case 0x4c:
	case 0x4d:
	case 0x4e:
	    modname.push_back(filename[i]);
	    loadcoff(afile);
	    break;
	case 0x21:
	    loadCoffLib(afile,filename[i]);
	    break;
	default:
	    printf("Unknown file type\n");
	    fclose(afile);
	    exit(1);
	}
	fclose(afile);
    }
}

void generateMap()
{
    long i,j;
    PPUBLIC q;
    SORTLIST::iterator l;

    afile=fopen(mapname.c_str(),"wt");
    if(!afile)
    {
	printf("Error opening map file %s\n",mapname.c_str());
	exit(1);
    }
    printf("Generating map file %s\n",mapname.c_str());

    for(i=0;i<seglist.size();i++)
    {
	if(seglist[i])
	{
	    fprintf(afile,"SEGMENT %s ", seglist[i]->name.c_str());
	    switch(seglist[i]->attr&SEG_COMBINE)
	    {
	    case SEG_PRIVATE:
		fprintf(afile,"PRIVATE ");
		break;
	    case SEG_PUBLIC:
		fprintf(afile,"PUBLIC ");
		break;
	    case SEG_PUBLIC2:
		fprintf(afile,"PUBLIC(2) ");
		break;
	    case SEG_STACK:
		fprintf(afile,"STACK ");
		break;
	    case SEG_COMMON:
		fprintf(afile,"COMMON ");
		break;
	    case SEG_PUBLIC3:
		fprintf(afile,"PUBLIC(3) ");
		break;
	    default:
		fprintf(afile,"unknown ");
		break;
	    }
	    if(seglist[i]->attr&SEG_USE32)
	    {
		fprintf(afile,"USE32 ");
	    }
	    else
	    {
		fprintf(afile,"USE16 ");
	    }
	    switch(seglist[i]->attr&SEG_ALIGN)
	    {
	    case SEG_ABS:
		fprintf(afile,"AT 0%04lXh ",seglist[i]->absframe);
		break;
	    case SEG_BYTE:
		fprintf(afile,"BYTE ");
		break;
	    case SEG_WORD:
		fprintf(afile,"WORD ");
		break;
	    case SEG_PARA:
		fprintf(afile,"PARA ");
		break;
	    case SEG_PAGE:
		fprintf(afile,"PAGE ");
		break;
	    case SEG_DWORD:
		fprintf(afile,"DWORD ");
		break;
	    case SEG_MEMPAGE:
		fprintf(afile,"MEMPAGE ");
		break;
	    default:
		fprintf(afile,"unknown ");
	    }
	    if(seglist[i]->classindex>=0)
		fprintf(afile,"'%s'\n",namelist[seglist[i]->classindex].c_str());
	    else
		fprintf(afile,"\n");
	    fprintf(afile,"  at %08lX, length %08lX\n",
		    (output_type == OUTPUT_PE) ? 
		    seglist[i]->getbase() + objectAlign + imageBase :
		    seglist[i]->getbase(),
		    seglist[i]->length);
	}
    }
    for(i=0;i<grplist.size();i++)
    {
	if(!grplist[i]) continue;
	fprintf(afile,"\nGroup %s:\n",grplist[i]->name.c_str());
	for(j=0;j<grplist[i]->numsegs;j++)
	{
	    fprintf(afile,"    %s\n",grplist[i]->segindex[j]->name.c_str());
	}
    }

    if(publics.size())
    {
	fprintf(afile,"\npublics:\n");
    }
    for(l=publics.begin();l!=publics.end();++l)
    {
	for(j=0;j<l->second.size();++j)
	{
	    q=(PPUBLIC)l->second[j];
	    if(q->modnum) continue;
	    fprintf(afile,"%s at %s:%08lX\n",
		    l->first.c_str(),
		    (q->seg) ? q->seg->name.c_str() : "Absolute",
		    q->ofs);
	}
    }

    printRelocs(afile);

    if(expdefs.size())
    {
	fprintf(afile,"\n %li exports:\n",expdefs.size());
	for(i=0;i<expdefs.size();i++)
	{
	    fprintf(afile,"%s(%i)=%s\n",expdefs[i].exp_name.c_str(),expdefs[i].ordinal,expdefs[i].int_name.c_str());
	}
    }
    if(impdefs.size())
    {
	fprintf(afile,"\n %li imports:\n",impdefs.size());
	for(i=0;i<impdefs.size();i++)
	{
	    fprintf(afile,"%s=%s:%s(%i)\n",impdefs[i].int_name.c_str(),impdefs[i].mod_name.c_str(),impdefs[i].flags==0?impdefs[i].imp_name.c_str():"",
		    impdefs[i].flags==0?0:impdefs[i].ordinal);
	}
    }
    fclose(afile);
}

int main(int argc,char *argv[])
{
    long i,j;
    int isend;
    char *libList;
    PPUBLIC q;

    buf.resize(65536);

    printf("ALINK v1.6 (C) Copyright 1998-9 Anthony A.J. Williams.\n");
    printf("This version of alink was modified for use in ReactOS-ppc "
	   "with Mr Williams' permission\n");
    printf("All Rights Reserved\n\n");

    libList=getenv("LIB");
    if(libList)
    {
	for(i=0,j=0;;i++)
	{
	    isend=(!libList[i]);
	    if(libList[i]==';' || !libList[i])
	    {
		if(i-j)
		{
		    if(libList[i-1]==PATH_CHAR)
		    {
			libPath.push_back(libList+j);
		    }
		    else
		    {
			std::string lp = libList+j;
			lp += PATH_CHAR;
			libPath.push_back(lp);
		    }
		}
		j=i+1;
	    }
	    if(isend) break;
	}
    }

    processArgs(argc,argv);

    if(!filename.size())
    {
	printf("No files specified\n");
	exit(1);
    }

    if(outname == "")
    {
	outname = filename[0];
	i=outname.size();
	while((i>=0)&&(outname[i]!='.')&&(outname[i]!=PATH_CHAR)&&(outname[i]!=':'))
	{
	    i--;
	}
	if(outname[i]=='.')
	{
	    outname[i]=0;
	}
    }
    i=outname.size();
    while((i>=0)&&(outname[i]!='.')&&(outname[i]!=PATH_CHAR)&&(outname[i]!=':'))
    {
	i--;
    }
    if(outname[i]!='.')
    {
	switch(output_type)
	{
	case OUTPUT_EXE:
	case OUTPUT_PE:
	    if(!buildDll)
	    {
		outname += ".exe";
	    }
	    else
	    {
		outname += ".dll";
	    }
	    break;
	case OUTPUT_COM:
	    outname += ".com";
	    break;
	default:
	    break;
	}
    }

    if(mapfile)
    {
	if(!mapname.size())
	{
	    mapname = outname;
	    while((i>=0)&&(mapname[i]!='.')&&(mapname[i]!=PATH_CHAR)&&(mapname[i]!=':'))
	    {
		i--;
	    }
	    if(mapname[i]!='.')
	    {
		i=mapname.size();
	    }
	    mapname += ".map";
	}
    }
    else
    {
	if(mapname.size())
	{
	    mapname="";
	}
    }

    loadFiles();

    if(!modname.size())
    {
	printf("No required modules specified\n");
	exit(1);
    }

    if(resource.size() && (output_type!=OUTPUT_PE))
    {
	printf("Cannot link resources into a non-PE application\n");
	exit(1);
    }

    if(entryPoint.size())
    {
	if(!case_sensitive)
	{
	    strupr(entryPoint);
	}
	
	if(gotstart)
	{
	    printf("Warning, overriding entry point from Command Line\n");
	}
	/* define an external reference for entry point */
	externs.push_back(EXTREC(entryPoint));

	/* point start address to this external */
	startaddr.ftype=REL_EXTFRAME; /* REL_EXTDISP ? */
	startaddr.frame=externs.size()-1;
	startaddr.ttype=REL_EXTONLY;
	startaddr.target=externs.size()-1;

	gotstart=TRUE;
    }

    matchExterns();
    printf("matched Externs\n");
    
    matchComDefs();
    printf("matched ComDefs\n");

    for(i=0;i<expdefs.size();i++)
    {
	if(!expdefs[i].pubdef)
	{
	    printf("Unresolved export %s=%s\n",expdefs[i].exp_name.c_str(),expdefs[i].int_name.c_str());
	    errcount++;
	}
	else if(expdefs[i].pubdef->aliasName.c_str() != "")
	{
	    printf("Unresolved export %s=%s, with alias %s\n",expdefs[i].exp_name.c_str(),expdefs[i].int_name.c_str(),(char *)expdefs[i].pubdef->aliasName.c_str());
	    errcount++;
	}
	
    }

    for(i=0;i<externs.size();i++)
    {
	if(externs[i].flags==EXT_NOMATCH)
	{
	    printf("Unresolved external %s\n",(char *)externs[i].name.c_str());
	    errcount++;
	}
	else if(externs[i].flags==EXT_MATCHEDPUBLIC)
	{
	    if(externs[i].pubdef->aliasName != "")
	    {
		printf("Unresolved external %s with alias %s\n",externs[i].name.c_str(),externs[i].pubdef->aliasName.c_str());
		errcount++;
	    }
	}
    }

    if(errcount!=0)
    {
	exit(1);
    }

    combineBlocks();
    sortSegments();

    if(mapfile) generateMap();
    switch(output_type)
    {
    case OUTPUT_COM:
	OutputCOMfile(outname);
	break;
    case OUTPUT_EXE:
	OutputEXEfile(outname);
	break;
    case OUTPUT_PE:
	OutputWin32file(outname);
	break;
    default:
	printf("Invalid output type\n");
	exit(1);
	break;
    }
    return 0;
}
