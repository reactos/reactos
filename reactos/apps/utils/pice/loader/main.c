/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    main.c

Abstract:
	
    loader/translator for pIce LINUX

Environment:

    User mode only

Author:

    Klaus P. Gerlicher

Revision History:

    04-Aug-1998:	created
    15-Nov-2000:    general cleanup of source files
    
Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

///////////////////////////////////////////////////////////////////////////////////
// includes
#include "stdinc.h"

#include <linux/errno.h>
#include <asm/ioctl.h>
#include <sys/stat.h>


///////////////////////////////////////////////////////////////////////////////////
// constant defines                                                                   


///////////////////////////////////////////////////////////////////////////////////
// global variables
char SrcFileNames[2048][2048];
ULONG ulCurrentSrcFile = 0;

int debugger_file;

ULONG ulGlobalVerbose = 0;


///////////////////////////////////////////////////////////////////////////////////
// process_stabs()
//
///////////////////////////////////////////////////////////////////////////////////
void process_stabs(
	char* pExeName,	// name of exe
	int fileout,	// symbol file handle
	Elf32_Shdr* pSHdr,
	int	nSHdrSize,
	void* p,		// ptr to memory where whole exe was read
	PSTAB_ENTRY pStab,	// ptr to stabs	
	int nStabLen,		// size of stabs
	char* pStr,			// ptr to stabs strings
	int nStrLen,		// sizeof stabs strings
	char* pGlobals,		// ptr to global symbols
	int nGlobalLen,		// sizeof of globals
	char* pGlobalsStr,	// ptr to global strings
	int nGlobalStrLen)	// size of global strings
{
    int i,strLen;
    int nOffset=0,nNextOffset=0;
    PSTAB_ENTRY pStabCopy = pStab;
    char* pName,szCurrentPath[2048];
	PICE_SYMBOLFILE_HEADER SymbolFileHeader;
	LPSTR pSlash,pDot;
	char temp[2048];
	char* pCopyExeName = temp;

    //printf("LOADER: enter process_stabs()\n");

    memset((void*)&SymbolFileHeader,0,sizeof(SymbolFileHeader));
	SymbolFileHeader.magic = PICE_MAGIC;
	strcpy(temp,pExeName);
	pSlash = strrchr(temp,'/');
	pDot = strrchr(temp,'.');
	if(pDot)
	{
		*pDot = 0;
	}
	if(pSlash)
	{
		pCopyExeName = pSlash+1;
	}
	strcpy(SymbolFileHeader.name,pCopyExeName);

    for(i=0;i<(nStabLen/sizeof(STAB_ENTRY));i++)
    {
        pName = &pStr[pStabCopy->n_strx + nOffset];

#if 0
        //printf("LOADER: \n%.8x %.2x %.2x %.4x %.8x %s\n",
                pStabCopy->n_strx,
                pStabCopy->n_type,
                pStabCopy->n_other,
                pStabCopy->n_desc,
                pStabCopy->n_value,
                pName
                );
#endif
        switch(pStabCopy->n_type)
        {
            case N_UNDF:
                nOffset += nNextOffset;
                nNextOffset = pStabCopy->n_value;
                //printf("LOADER: changing string offset %x %x\n",nOffset,nNextOffset);
                break;
            case N_SO:
                if((strLen = strlen(pName)))
                {
                    if(pName[strLen-1]!='/')
                    {
                        if(strlen(szCurrentPath))
                        {
                            //printf("LOADER: ###########################################################################\n");
                            strcat(szCurrentPath,pName);
                            //printf("LOADER: changing source file %s\n",szCurrentPath);
                            strcpy(SrcFileNames[ulCurrentSrcFile++],szCurrentPath);
                            szCurrentPath[0]=0;
                        }
                        else
                        {
                            //printf("LOADER: ###########################################################################\n");
                            //printf("LOADER: changing source file %s\n",pName);
                            strcpy(SrcFileNames[ulCurrentSrcFile++],pName);
                        }
                    }
                    else
                        strcpy(szCurrentPath,pName);
                }
                else
                {
                    //printf("LOADER: END source file\n");
                    //printf("LOADER: ###########################################################################\n");
                }
                break;
/*            case N_SLINE:
                //printf("LOADER: code source line number #%u for addr. %x\n",pStabCopy->n_desc,pStabCopy->n_value);
                break;
            case N_DSLINE:
                //printf("LOADER: data source line number #%u for addr. %x\n",pStabCopy->n_desc,pStabCopy->n_value);
                break;
            case N_BSLINE:
                //printf("LOADER: BSS source line number #%u for addr. %x\n",pStabCopy->n_desc,pStabCopy->n_value);
                break;
            case N_GSYM:
                //printf("LOADER: global symbol %s @ addr. %x (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                break;
            case N_BINCL:
                //printf("LOADER: include file %s\n",pName);
                break;
            case N_EINCL:
                break;
            case N_FUN:
                if(strlen(pName))
                    //printf("LOADER: function %s @ addr. %x (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                else
                    //printf("LOADER: text segment %x (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                break;
            case N_PSYM:
                //printf("LOADER: parameter %s @ [EBP%+d] (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                break;
            case N_RSYM:
                //printf("LOADER: register variable %s @ reg. %x (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                break;
            case N_LBRAC:
                //printf("LOADER: lexical block %s @ reg. %x (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                break;
            case N_RBRAC:
                //printf("LOADER: END of lexical block %s @ reg. %x (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                break;
            case N_STSYM:
                //printf("LOADER: static variable %s @ %x (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                break;
            case N_LCSYM:
                //printf("LOADER: BSS variable %s @ %x (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                break;
            case N_LSYM:
                if(pStabCopy->n_value)
                {
                    //printf("LOADER: stack variable %s @ [EBP%+d] (%x)\n",pName,pStabCopy->n_value,pStabCopy->n_desc);
                }
                else
                {
                    //printf("LOADER: global variable %s \n",pName);
                }
                break;
*/
        }

        pStabCopy++;
    }

	//printf("LOADER: SymbolFileHeader.ulSizeOfHeader= %x (%x)\n",nSHdrSize,(LPSTR)pSHdr-(LPSTR)p);
	//printf("LOADER: SymbolFileHeader.ulSizeOfGlobals = %x (%x)\n",nGlobalLen,(LPSTR)pGlobals-(LPSTR)p);
	//printf("LOADER: SymbolFileHeader.ulSizeOfGlobalsStrings = %x (%x)\n",nGlobalStrLen,(LPSTR)pGlobalsStr-(LPSTR)p);
	//printf("LOADER: SymbolFileHeader.ulSizeOfStabs = %x (%x)\n",nStabLen,(LPSTR)pStab-(LPSTR)p);
	//printf("LOADER: SymbolFileHeader.ulSizeOfStabsStrings = %x (%x)\n",nStrLen,(LPSTR)pStr-(LPSTR)p);

	SymbolFileHeader.ulOffsetToHeaders = sizeof(PICE_SYMBOLFILE_HEADER);
	SymbolFileHeader.ulSizeOfHeader = nSHdrSize;
	SymbolFileHeader.ulOffsetToGlobals = sizeof(PICE_SYMBOLFILE_HEADER)+nSHdrSize;
	SymbolFileHeader.ulSizeOfGlobals = nGlobalLen;
	SymbolFileHeader.ulOffsetToGlobalsStrings = sizeof(PICE_SYMBOLFILE_HEADER)+nSHdrSize+nGlobalLen;
	SymbolFileHeader.ulSizeOfGlobalsStrings = nGlobalStrLen;
	SymbolFileHeader.ulOffsetToStabs = sizeof(PICE_SYMBOLFILE_HEADER)+nSHdrSize+nGlobalLen+nGlobalStrLen;
	SymbolFileHeader.ulSizeOfStabs = nStabLen;
	SymbolFileHeader.ulOffsetToStabsStrings = sizeof(PICE_SYMBOLFILE_HEADER)+nSHdrSize+nGlobalLen+nGlobalStrLen+nStabLen;
	SymbolFileHeader.ulSizeOfStabsStrings = nStrLen;
    SymbolFileHeader.ulOffsetToSrcFiles = sizeof(PICE_SYMBOLFILE_HEADER)+nSHdrSize+nGlobalLen+nGlobalStrLen+nStabLen+nStrLen;
    SymbolFileHeader.ulNumberOfSrcFiles = ulCurrentSrcFile;

	write(fileout,&SymbolFileHeader,sizeof(SymbolFileHeader));

	write(fileout,pSHdr,nSHdrSize);
	write(fileout,pGlobals,nGlobalLen);
	write(fileout,pGlobalsStr,nGlobalStrLen);
	write(fileout,pStab,nStabLen);
	write(fileout,pStr,nStrLen);

    for(i=0;i<ulCurrentSrcFile;i++)
    {
        int file;
        int len;
        PVOID pFile;
        PICE_SYMBOLFILE_SOURCE pss;
    
        file = open(SrcFileNames[i],O_RDONLY);
        if(file>0)
        {
            //printf("LOADER: [%u] opened %s as FD %x\n",i,SrcFileNames[i],file);

            len = lseek(file,0,SEEK_END);
            //printf("LOADER: length = %x\n",(int)len);
        
            lseek(file,0,SEEK_SET);

            strcpy(pss.filename,SrcFileNames[i]);
            pss.ulOffsetToNext = len+sizeof(PICE_SYMBOLFILE_SOURCE);

            pFile = malloc(len);
            //printf("LOADER: memory for file @ %x\n",pFile);
            if(pFile)
            {
                //printf("LOADER: reading file...\n");
                read(file,pFile,len);
    
                write(fileout,&pss,sizeof(PICE_SYMBOLFILE_SOURCE));
                //printf("LOADER: writing file...\n");
                write(fileout,pFile,len);
                free(pFile);
            }

            close(file);
        }

    }

    //printf("LOADER: leave process_stabs()\n");
}

///////////////////////////////////////////////////////////////////////////////////
// find_stab_sections()
//
///////////////////////////////////////////////////////////////////////////////////
void find_stab_sections(void* p,Elf32_Shdr* pSHdr,PSTAB_ENTRY* ppStab,int* pLen,char** ppStr,int* pnStabStrLen,int num,int index)
{
int i;
char* pStr = (char*)((int)p + pSHdr[index].sh_offset);

    //printf("LOADER: enter find_stab_sections()\n");
    *ppStab = 0;
    *ppStr = 0;
    for(i=0;i<num;i++,pSHdr++)
    {
        int sh_name = pSHdr->sh_name;
        //printf("LOADER: [%u] %32s %8x %8x %8x %8x %8x\n",i,&pStr[sh_name],pSHdr->sh_offset,pSHdr->sh_size,pSHdr->sh_addr,pSHdr->sh_type,pSHdr->sh_link);
        if(strcmp(&pStr[sh_name],".stab") == 0)
        {
            *ppStab = (PSTAB_ENTRY)((int)p + pSHdr->sh_offset);
            *pLen = pSHdr->sh_size;
            //printf("LOADER: .stab @ %x (offset %x) len = %x\n",*ppStab,pSHdr->sh_offset,pSHdr->sh_size);
        }
        else if(strcmp(&pStr[sh_name],".stabstr") == 0)
        {
            *ppStr = (char*)((int)p + pSHdr->sh_offset);
			*pnStabStrLen = pSHdr->sh_size;
            //printf("LOADER: .stabstr @ %x (offset %x size=%u)\n",*ppStr,pSHdr->sh_offset,pSHdr->sh_size);
        }
    }

    //printf("LOADER: leave find_stab_sections()\n");
}

///////////////////////////////////////////////////////////////////////////////////
// find_symtab()
//
///////////////////////////////////////////////////////////////////////////////////
Elf32_Sym* find_symtab(void* p,Elf32_Shdr* pSHdrOrig,int num,int index,int* pLen,LPSTR* ppStr,int *pnSymStrLen)
{
    int i;
    Elf32_Sym* pSym = NULL,*pSymOrig = NULL;
//    char* pStr = (char*)((int)p + pSHdrOrig[index].sh_offset);
    LPSTR pName;
    ULONG ulSymTabEntries = 0,link=-1;
    Elf32_Shdr* pSHdr;

    //printf("LOADER: enter find_symtab()\n");

    // find global symbol table
    pSHdr = pSHdrOrig;
    for(i=0;i<num;i++,pSHdr++)
    {
        //int sh_name = pSHdr->sh_name;
        //printf("LOADER: [%u] %32s %8x %8x %8x %8x %8x\n",i,pStr,pSHdr->sh_offset,pSHdr->sh_size,pSHdr->sh_addr,pSHdr->sh_type,pSHdr->sh_link);
		if(pSHdr->sh_type == SHT_SYMTAB)
		{
			pSym = (Elf32_Sym*)((int)p+pSHdr->sh_offset);

			//printf("LOADER: symbol table %u %x %u\n",i,pSHdr->sh_offset,pSHdr->sh_link);
			ulSymTabEntries = pSHdr->sh_size;
			link = pSHdr->sh_link;
		}
    }

    if(link != (-1))
    {
        // find global string table
        pSHdr = pSHdrOrig;
        for(i=0;i<num;i++,pSHdr++)
        {
            //int sh_name = pSHdr->sh_name;
            //printf("LOADER: [%u] %32s %8x %8x %8x %8x %8x\n",i,pStr,pSHdr->sh_offset,pSHdr->sh_size,pSHdr->sh_addr,pSHdr->sh_type,pSHdr->sh_link);
		    if(pSHdr->sh_type == SHT_STRTAB && i==link)
		    {
			    *ppStr = (LPSTR)((int)p+pSHdr->sh_offset);
				*pnSymStrLen = pSHdr->sh_size;
		    }
	    }

	    if(*ppStr && pSym)
	    {
		    LPSTR pStr = *ppStr;

			pSymOrig = pSym;
		    for(i=0;i<ulSymTabEntries/sizeof(Elf32_Sym);i++)
		    {
			    pName = &pStr[pSym->st_name];
			    //printf("LOADER: [%u] %32s %x %x %x %x\n",i,pName,pSym->st_name,pSym->st_value,pSym->st_info,pSym->st_other);
			    pSym++;
		    }
	    }
	    *pLen = ulSymTabEntries;
    }
    else
    {
        pSymOrig= NULL;
    }

    //printf("LOADER: leave find_symtab()\n");

	return pSymOrig;
}

///////////////////////////////////////////////////////////////////////////////////
// process_elf()
//
///////////////////////////////////////////////////////////////////////////////////
int process_elf(char* filename,int file,void* p,int len)
{
	Elf32_Ehdr* pEHdr =(Elf32_Ehdr*)p;
	Elf32_Shdr* pSHdr;
	char* pStr;
	PSTAB_ENTRY pStab;
	int nStabLen,nSym;
	LPSTR pStrTab;
	Elf32_Sym* pSymTab;
	char szSymName[2048];
	int fileout;
	int nSymStrLen,nStabStrLen;
    int iRetVal = 0;

    //printf("LOADER: enter process_elf()\n");
    if(strncmp(pEHdr->e_ident,"\177ELF",4) == 0) // is ELF binary magic
    {
        pSHdr = (Elf32_Shdr*)((int)p+pEHdr->e_shoff);
        //printf("LOADER: Section header @ %x (offset %x)\n",pSHdr,pEHdr->e_shoff);
        //printf("LOADER: %u entries\n",pEHdr->e_shnum);
        //printf("LOADER: string table index %u\n",pEHdr->e_shstrndx);
		if((pSymTab = find_symtab(p,pSHdr,pEHdr->e_shnum,pEHdr->e_shstrndx,&nSym,&pStrTab,&nSymStrLen)) != NULL )
		{
			find_stab_sections(p,pSHdr,&pStab,&nStabLen,&pStr,&nStabStrLen,pEHdr->e_shnum,pEHdr->e_shstrndx);
			if(pStab && nStabLen && pStr && nStabStrLen)
			{
				LPSTR pDot;

				strcpy(szSymName,filename);
				//printf("LOADER: file name = %s\n",szSymName);
				if((pDot = strrchr(szSymName,'.')))
				{
					*pDot = 0;
					strcat(pDot,".sym");
				}
				else
				{
					strcat(szSymName,".sym");
				}
				//printf("LOADER: symbol file name = %s\n",szSymName);
                printf("LOADER: creating symbol file %s for %s\n",szSymName,filename);

                fileout = creat(szSymName,S_IRUSR|S_IWUSR);     // make r/w for owner
                if(fileout != -1)				
				{
					process_stabs(szSymName,
								  fileout,
								  pSHdr,
								  pEHdr->e_shnum*sizeof(Elf32_Shdr),
								  p,
								  pStab,
								  nStabLen,
								  pStr,
								  nStabStrLen,
								  (LPSTR)pSymTab,
								  nSym,
								  pStrTab,
								  nSymStrLen);

					close(fileout);
				}
                else
                {
                    printf("LOADER: creation of symbol file %s failed\n",szSymName);
					iRetVal = 2;
                }

			}
            else                                                          
            {                                                             
                printf("LOADER: file %s has no data inside symbol tables\n",filename);
				if( ulGlobalVerbose )
				{
                    if( !pStab || !nStabLen )                             
                        printf("LOADER: - symbol table is empty or not present\n");
                    if( !pStr  || !nStabStrLen )                          
                        printf("LOADER: - string table is empty or not present\n");
				}
                iRetVal = 2;                                          
            }                                                             
        }                                                                     
        else                                                                  
        {                                                                     
            printf("LOADER: file %s does not have a symbol table\n",filename);             
            iRetVal = 2;
        }
    }
    else                                                                          
    {                                                                             
        printf("LOADER: file %s is not an ELF binary\n",filename);                    
        iRetVal = 1;                                                          
    }
	   
    //printf("LOADER: leave process_elf()\n");
    return iRetVal;
}

///////////////////////////////////////////////////////////////////////////////////
// process_file()
//
///////////////////////////////////////////////////////////////////////////////////
int process_file(char* filename)
{
	int file;
	void* p;
	off_t len;
	int iRetVal=0;

    //printf("LOADER: enter process_file()\n");
    file = open(filename,O_RDONLY);
    if(file>0)
    {
        //printf("LOADER: opened %s as FD %x\n",filename,file);

        len = lseek(file,0,SEEK_END);
        printf("LOADER: file %s is %u bytes\n",filename,(int)len);
        
        lseek(file,0,SEEK_SET);

        p = malloc(len+16);
        if(p)
        {
            //printf("LOADER: malloc'd @ %x\n",p);
            memset(p,0,len+16);

            if(len == read(file,p,len))
            {
                //printf("LOADER: trying ELF format\n");
                iRetVal = process_elf(filename,file,p,len);
            }
        }

        close(file);
    }
    else
    {
        printf("LOADER: file %s could not be opened\n",filename);
		iRetVal = 1;
    }

    //printf("LOADER: leave process_file()\n");
    return iRetVal;
}

///////////////////////////////////////////////////////////////////////////////////
// open_debugger()
//
///////////////////////////////////////////////////////////////////////////////////
int	open_debugger(void)
{
    debugger_file = open("/dev/pice0",O_RDONLY);
	if(debugger_file<0)
	{
		printf("LOADER: debugger is not loaded\n");
	}

	return debugger_file;
}

///////////////////////////////////////////////////////////////////////////////////
// close_debugger()
//
///////////////////////////////////////////////////////////////////////////////////
void close_debugger(void)
{
	close(debugger_file);
}

///////////////////////////////////////////////////////////////////////////////////
// banner()
//
///////////////////////////////////////////////////////////////////////////////////
void banner(void)
{
    printf("#########################################################\n");
    printf("####       Symbols LOADER/TRANSLATOR for PICE        ####\n");
    printf("#########################################################\n");
}

#define ACTION_NONE             0
#define ACTION_LOAD             1
#define ACTION_UNLOAD           2
#define ACTION_TRANS            3
#define ACTION_RELOAD           4
#define ACTION_INSTALL          5
#define ACTION_UNINSTALL        6
#define ACTION_STATUS           7
#define ACTION_BREAK            8
#define ACTION_TERMINAL         9

///////////////////////////////////////////////////////////////////////////////////
// change_symbols()
//
///////////////////////////////////////////////////////////////////////////////////
void change_symbols(int action,char* pfilename)
{
    int iRetVal = 0;
	
	switch(action)
	{
		case ACTION_LOAD:
			printf("LOADER: loading symbols from %s\n",pfilename);
			if(open_debugger()>=0)
			{
				iRetVal = ioctl(debugger_file,PICE_IOCTL_LOAD,pfilename);
				close_debugger();
			}
			break;
		case ACTION_UNLOAD:
			printf("LOADER: unloading symbols from %s\n",pfilename);
			if(open_debugger()>=0)
			{
				iRetVal = ioctl(debugger_file,PICE_IOCTL_UNLOAD,pfilename);
				close_debugger();
			}
			break;
		case ACTION_RELOAD:
			printf("LOADER: reloading all symbols\n");
			if(open_debugger()>=0)
			{
				ioctl(debugger_file,PICE_IOCTL_RELOAD,NULL);
				close_debugger();
    			printf("LOADER: reloading DONE!\n");
			}
			break;
        default :
            printf("LOADER: an internal error has occurred at change_symbols\n");
	}
	
    switch( iRetVal )
	{
	    case -EINVAL :
			printf("LOADER: debugger return value = -EINVAL, operation has failed\n");
			break;
		case 0 :
			// success - silently proceed
			break;
		default :
			printf("LOADER: debugger return value = %i, operation possibly failed\n",iRetVal);
	}
}

///////////////////////////////////////////////////////////////////////////////////
// tryinstall()
//
///////////////////////////////////////////////////////////////////////////////////
int tryinstall(void)
{
    char *argv[]={"/sbin/insmod","pice.o",NULL};
    int err = 0;
    int pid,status;
    
    banner();
    printf("LOADER: trying to install debugger...\n");

    if(!(open_debugger() < 0) )
    {
        printf("LOADER: debugger already installed...\n");
        close_debugger();
        return 0;
    }

    // create a separate thread
    pid = fork();
    switch(pid)
    {
        case -1:
            // error when forking, i.e. out E_NOMEM
            err = errno;
            printf("LOADER: fork failed for execution of '%s' (errno = %u).\n",argv[0],err);
            break;
        case 0:
            // child process handler 
            execve(argv[0],argv,NULL);
            // returns only on error, with return value -1, errno is set
            printf("LOADER: couldn't execute '%s' (errno = %u)\n",argv[0],errno);
            exit(255);
            break;
        default:
            // parent process handler
            printf("LOADER: waiting for debugger to load...\n");
            pid = waitpid(pid, &status, 0); // suspend until child is done
            if( (pid>0) && WIFEXITED(status) && (WEXITSTATUS(status) == 0) )
                printf("LOADER: debugger loaded!\n");
            else if( pid<=0 )
            {
                printf("LOADER: Error on loading debugger! (waitpid() = %i)\n",pid);
                err = -1;
            }
            else if( !WIFEXITED(status) )
            {
                printf("LOADER: Error on loading debugger! (ifexited = %i)\n",WIFEXITED(status));
                err = -1;
            }
            else
            {
                printf("LOADER: Error on loading debugger! (exitstatus = %u)\n",WEXITSTATUS(status));
                err = WEXITSTATUS(status);
            }
            break;
    }
    
    return err;
}

///////////////////////////////////////////////////////////////////////////////////
// tryuninstall()
//
///////////////////////////////////////////////////////////////////////////////////
int tryuninstall(void)
{
    char *argv[]={"/sbin/rmmod","pice",NULL};
    int err = 0;
    int pid,status;
    
    banner();
    printf("LOADER: trying to remove debugger...\n");
    
    // check for loaded debugger
    if(open_debugger() < 0)
    {
        return -1;
    }
    // don't to close, else we'll have a reference count != 0
    close_debugger();

    // create a separate thread
    pid = fork();
    switch(pid)
    {
        case -1:
            // error when forking, i.e. out E_NOMEM
            err = errno;
            printf("LOADER: fork failed for execution of '%s' (errno=%u).\n",argv[0],err);
            break;
        case 0:
            // child process handler 
            execve(argv[0],argv,NULL);
            // returns only on error, with return value -1, errno is set
            printf("LOADER: couldn't execute '%s' (errno = %u)\n",argv[0],errno);
            exit(255);
            break;
        default:
            // parent process handler
            printf("LOADER: waiting for debugger to unload...\n");
            pid = waitpid(pid, &status, 0); // suspend until child is done
            
            if( (pid>0) && WIFEXITED(status) && (WEXITSTATUS(status) == 0) )
                printf("LOADER: debugger removed!\n");
            else if( pid<=0 )
            {
                printf("LOADER: Error on removing debugger! (waitpid() = %i)\n",pid);
                err = -1;
            }
            else if( !WIFEXITED(status) )
            {
                printf("LOADER: Error on removing debugger! (ifexited = %i)\n",WIFEXITED(status));
                err = -1;
            }
            else
            {
                printf("LOADER: Error on removing debugger! (exitstatus = %u)\n",WEXITSTATUS(status));
                err = WEXITSTATUS(status);
            }
            break;
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////////
// showstatus()
//
///////////////////////////////////////////////////////////////////////////////////
void showstatus(void)
{
    DEBUGGER_STATUS_BLOCK sb;
    int iRetVal;

	if(open_debugger()>=0)
	{
        sb.Test = 0;
		iRetVal = ioctl(debugger_file,PICE_IOCTL_STATUS,&sb);

        printf("LOADER: Test = %X\n",sb.Test);
		close_debugger();
	}
}

///////////////////////////////////////////////////////////////////////////////////
// dobreak()
//
///////////////////////////////////////////////////////////////////////////////////
void dobreak(void)
{
    int iRetVal;

	if(open_debugger()>=0)
	{
		iRetVal = ioctl(debugger_file,PICE_IOCTL_BREAK,NULL);

		close_debugger();
	}
}

///////////////////////////////////////////////////////////////////////////////////
// doterminal()
//
///////////////////////////////////////////////////////////////////////////////////
void doterminal(void)
{
    if(SetupSerial(2,B115200))
    {
        DebuggerShell();
        CloseSerial();
    }
}

///////////////////////////////////////////////////////////////////////////////////
// process_switches()
//
// returns !=0 in case of a commandline error
//
///////////////////////////////////////////////////////////////////////////////////
int process_switches(int argc,char* argv[])
{
	int i;
	char* parg,*pfilename = NULL;
	int action = ACTION_NONE;
	int error = 0;

    // parse commandline arguments
	for(i=1;i<argc;i++)
	{
		parg = argv[i];
		if(*parg == '-')
		{
		    int new_action=ACTION_NONE;
			
			parg++;
			if(strcmp(parg,"load")==0 || strcmp(parg,"l")==0)
			{
				new_action = ACTION_LOAD;
			}
			else if(strcmp(parg,"unload")==0 || strcmp(parg,"u")==0)
			{
				new_action = ACTION_UNLOAD;
			}
			else if(strcmp(parg,"trans")==0 || strcmp(parg,"t")==0)
            {
                new_action = ACTION_TRANS;
            }
			else if(strcmp(parg,"reload")==0 || strcmp(parg,"r")==0)
            {
                new_action = ACTION_RELOAD;
            }
			else if(strcmp(parg,"verbose")==0 || strcmp(parg,"v")==0)
            {
			    if( ulGlobalVerbose+1 > ulGlobalVerbose )
				    ulGlobalVerbose++;
            }
			else if(strcmp(parg,"install")==0 || strcmp(parg,"i")==0)
            {
                new_action = ACTION_INSTALL;
            }
			else if(strcmp(parg,"uninstall")==0 || strcmp(parg,"x")==0)
            {
                new_action = ACTION_UNINSTALL;
            }
			else if(strcmp(parg,"status")==0 || strcmp(parg,"s")==0)
            {
                new_action = ACTION_STATUS;
            }
			else if(strcmp(parg,"break")==0 || strcmp(parg,"b")==0)
            {
                new_action = ACTION_BREAK;
            }
			else if(strcmp(parg,"serial")==0 || strcmp(parg,"ser")==0)
            {
                new_action = ACTION_TERMINAL;
            }
			else
			{
				printf("LOADER: error: unknown switch %s", argv[i]);
				error = 1;
			}

            if( new_action != ACTION_NONE )                               
            {                                                             
                if( action == ACTION_NONE )                           
                    action = new_action;                          
                else                                                  
                if( action == new_action )                            
                {                                                     
                    // identical, just ignore                     
                }                                                     
                else                                                  
                {                                                     
                    printf("LOADER: error: conflicting switch %s", argv[i]);
                    error = 1;                                    
                }                                                     
            }
		}
		else
		{
            if( pfilename )                                               
            {                                                             
                printf("LOADER: error: additional filename %s", parg);        
                error = 1;                                            
            }
			pfilename = parg;
		}
	}

    // check number of required parameters
    switch( action )
    {                                                                              
        case ACTION_TRANS :                                                    
        case ACTION_LOAD :                                                     
        case ACTION_UNLOAD :                                                   
            if( !pfilename )                                               
            {                                                              
                printf("LOADER: error: missing filename\n");                   
                error = 1;                                             
            }                                                              
            break;                                                         
        case ACTION_RELOAD :                                                   
            /* filename parameter is optional */     
            break;
        case ACTION_UNINSTALL:
            close_debugger();
            tryuninstall();
            break;                                                         
        case ACTION_INSTALL:
            tryinstall();
            break;           
        case ACTION_STATUS:
            showstatus();
            break;
        case ACTION_BREAK:
            dobreak();
            break;
        case ACTION_TERMINAL:
            doterminal();
            break;
        case ACTION_NONE :                                                     
            printf("LOADER: no action specified specifed on commandline\n");       
            error = 1;                                                     

            break;
        default :
            printf("LOADER: an internal error has occurred at commandline parsing\n");
            error = 1;
    }
																																																																																																																					
    if( !error )    // commandline was fine, now start processing
    {
        switch( action )
        {
            case ACTION_TRANS :
                printf("LOADER: trying to translate file %s...\n",pfilename);
                if( process_file(pfilename)==0 )
                    printf("LOADER: file %s has been translated\n",pfilename);
                else                                                  
                    printf("LOADER: error while translating file %s\n",pfilename);
                break;                                                
            case ACTION_LOAD :                                            
            case ACTION_UNLOAD :                                          
            case ACTION_RELOAD :                                          
                change_symbols(action,pfilename);                     
                break;                                                
        }                                                                            
    }            

    return error;
}


///////////////////////////////////////////////////////////////////////////////////
// showhelp()
//
///////////////////////////////////////////////////////////////////////////////////
void showhelp(void)
{
    banner();
    printf("LOADER: Syntax:\n");
    printf("LOADER:         loader [switches] [executable/object file path]\n");
    printf("LOADER: Switches:\n");
    printf("LOADER:         -trans      (-t):   translate from exe to sym\n");
    printf("LOADER:         -load       (-l):   load symbols\n");
    printf("LOADER:         -unload     (-u):   unload symbols\n");
    printf("LOADER:         -reload     (-r):   reload some/all symbols\n");
    printf("LOADER:         -verbose    (-v):   be a bit more verbose\n");
    printf("LOADER:         -install    (-i):   install pICE debugger\n");
    printf("LOADER:         -uninstall  (-x):   uninstall pICE debugger\n");
    printf("LOADER:         -break      (-b):   break into debugger\n");
    printf("LOADER:         -serial     (-ser): start serial line terminal\n");
}

///////////////////////////////////////////////////////////////////////////////////
// showpermission()
//
///////////////////////////////////////////////////////////////////////////////////
void showpermission(void)
{
    banner();
    printf("LOADER: You must be superuser!\n");
}

///////////////////////////////////////////////////////////////////////////////////
// main()
//
///////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
    int uid;

    // find out who's using us
    // if it's not the superuser, bail!

    if(argc==1 || argc>3)
    {
		showhelp();

		return 1;
    }
    else 
    {
        uid = getuid();
        if(uid != 0)
        {
            showpermission();
            return 0;
        }

        return process_switches(argc,argv);
    }
}
