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
	Reactos Port by Eugene Ingerman

Revision History:

    04-Aug-1998:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

///////////////////////////////////////////////////////////////////////////////////
// includes
#include "stdinc.h"
#include <wchar.h>

///////////////////////////////////////////////////////////////////////////////////
// constant defines


///////////////////////////////////////////////////////////////////////////////////
// global variables
char SrcFileNames[2048][2048];
ULONG ulCurrentSrcFile = 0;

HANDLE debugger_file;

ULONG ulGlobalVerbose = 0;


///////////////////////////////////////////////////////////////////////////////////
// process_stabs()
//
///////////////////////////////////////////////////////////////////////////////////
void process_stabs(
	char* pExeName,	// name of exe
	HANDLE fileout,	// symbol file handle
	PIMAGE_SECTION_HEADER section, //Elf32_Shdr* pSHdr,
	int sectionHeadersSize, //int	nSHdrSize,
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
    unsigned i,strLen;
    int nOffset=0,nNextOffset=0;
    PSTAB_ENTRY pStabCopy = pStab;
    char* pName,szCurrentPath[2048];
	PICE_SYMBOLFILE_HEADER SymbolFileHeader;
	LPSTR pSlash,pDot;
	char temp[2048];
	char* pCopyExeName = temp;
	WCHAR tempstr[64];
	DWORD wrote;

    //printf("LOADER: enter process_stabs()\n");

	//get the name of the executable file
    memset((void*)&SymbolFileHeader,0,sizeof(SymbolFileHeader));
	SymbolFileHeader.magic = PICE_MAGIC;
	strcpy(temp,pExeName);
	pSlash = strrchr(temp,'\\');
	pDot = strchr(temp,'.');
	if(pDot)
	{
		*pDot = 0;
	}
	if(pSlash)
	{
		pCopyExeName = pSlash+1;
	}
	strLen = MultiByteToWideChar(CP_ACP, NULL, pCopyExeName, -1, tempstr, 64 );
	if( !strLen )
		printf("Cannot convert string to multibyte: %s\n", pCopyExeName );
	wcscpy(SymbolFileHeader.name,tempstr);

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
	SymbolFileHeader.ulSizeOfHeader = sectionHeadersSize;
	SymbolFileHeader.ulOffsetToGlobals = sizeof(PICE_SYMBOLFILE_HEADER)+sectionHeadersSize;
	SymbolFileHeader.ulSizeOfGlobals = nGlobalLen;
	SymbolFileHeader.ulOffsetToGlobalsStrings = sizeof(PICE_SYMBOLFILE_HEADER)+sectionHeadersSize+nGlobalLen;
	SymbolFileHeader.ulSizeOfGlobalsStrings = nGlobalStrLen;
	SymbolFileHeader.ulOffsetToStabs = sizeof(PICE_SYMBOLFILE_HEADER)+sectionHeadersSize+nGlobalLen+nGlobalStrLen;
	SymbolFileHeader.ulSizeOfStabs = nStabLen;
	SymbolFileHeader.ulOffsetToStabsStrings = sizeof(PICE_SYMBOLFILE_HEADER)+sectionHeadersSize+nGlobalLen+nGlobalStrLen+nStabLen;
	SymbolFileHeader.ulSizeOfStabsStrings = nStrLen;
    SymbolFileHeader.ulOffsetToSrcFiles = sizeof(PICE_SYMBOLFILE_HEADER)+sectionHeadersSize+nGlobalLen+nGlobalStrLen+nStabLen+nStrLen;
    SymbolFileHeader.ulNumberOfSrcFiles = ulCurrentSrcFile;

	printf("sectionHeaderSize: %ld, nGlobalLen: %ld, nGlobalStrLen: %ld, nStabLen: %ld, "
			"nStrLen: %ld, ulCurrentSrcFile: %ld, ulOffsetToStabs: %ld\n",
			sectionHeadersSize, nGlobalLen, nGlobalStrLen,
			nStabLen, nStrLen, ulCurrentSrcFile, SymbolFileHeader.ulOffsetToStabs);

	WriteFile(fileout,&SymbolFileHeader,sizeof(PICE_SYMBOLFILE_HEADER),&wrote, NULL);
	WriteFile(fileout,section,sectionHeadersSize,&wrote, NULL);
	WriteFile(fileout,pGlobals,nGlobalLen,&wrote, NULL);
	WriteFile(fileout,pGlobalsStr,nGlobalStrLen,&wrote, NULL);
	WriteFile(fileout,pStab,nStabLen,&wrote, NULL);
	WriteFile(fileout,pStr,nStrLen,&wrote, NULL);

    for(i=0;i<ulCurrentSrcFile;i++)
    {
        HANDLE file;
        int len;
        PVOID pFile;
        PICE_SYMBOLFILE_SOURCE pss;

		file = CreateFile(SrcFileNames[i],GENERIC_READ , 0, NULL, OPEN_EXISTING, 0, 0);
		//printf("Trying To Open: %s, result: %x\n", SrcFileNames[i], file );


		if( file == INVALID_HANDLE_VALUE ){
			//let's try win format drive:/file
			char srctmp[2048];
			strcpy(srctmp, SrcFileNames[i] );
			if(strncmp(srctmp,"//",2)==0){
				*(srctmp) = *(srctmp+2);
				*(srctmp+1) = ':';
				*(srctmp+2) = '/';
				file = CreateFile(srctmp,GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
				//printf("Trying To Open: %s, handle: %x\n", srctmp, file );
				if( file == INVALID_HANDLE_VALUE )
					printf("Can't open file: %s\n", srctmp );
			}
		}
        if(file != INVALID_HANDLE_VALUE)
        {
            //printf("LOADER: [%u] opened %s as FD %x\n",i,SrcFileNames[i],file);

            len = SetFilePointer(file,0,NULL,FILE_END);
            //printf("LOADER: length = %d\n",(int)len);

            SetFilePointer(file,0,NULL,FILE_BEGIN);

            strcpy(pss.filename,SrcFileNames[i]);
            pss.ulOffsetToNext = len+sizeof(PICE_SYMBOLFILE_SOURCE);

            pFile = malloc(len+1);
            //printf("LOADER: memory for file @ %x\n",pFile);
            if(pFile)
            {
                //printf("LOADER: reading file...\n");
                ReadFile(file,pFile,len+1,&wrote,NULL);
				//printf("read: %d, error: %d\n", wrote, GetLastError());
                WriteFile(fileout,&pss,sizeof(PICE_SYMBOLFILE_SOURCE),&wrote, NULL);
                WriteFile(fileout,pFile,len,&wrote, NULL);
                //printf("LOADER: writing file...%d\n%s\n",wrote,pFile );
                free(pFile);
            }

            CloseHandle(file);
        }

    }

    //printf("LOADER: leave process_stabs()\n");
}

///////////////////////////////////////////////////////////////////////////////////
// find_stab_sections()
//
///////////////////////////////////////////////////////////////////////////////////
void find_stab_sections(void* p,PIMAGE_SECTION_HEADER section, unsigned cSections,
							  PSTAB_ENTRY* ppStab,int* pLen,char** ppStr,int* pnStabStrLen)
{
	unsigned i;
    //printf("LOADER: enter find_stab_sections()\n");
    *ppStab = 0;
    *ppStr = 0;

	for ( i=1; i <= cSections; i++, section++ )
    {

		if(strcmp(section->Name,".stab") == 0)
        {
            *ppStab = (PSTAB_ENTRY)((int)p + section->PointerToRawData);
            *pLen = section->SizeOfRawData;
            printf("LOADER: .stab @ %x (offset %x) len = %x\n",*ppStab,section->PointerToRawData,section->SizeOfRawData);
        }
        else if(strncmp(section->Name,".stabstr",strlen(".stabstr")) == 0)
        {
            *ppStr = (char*)((int)p + section->PointerToRawData);
			*pnStabStrLen = section->SizeOfRawData;
            printf("LOADER: .stabstr @ %x (offset %x) len = %x\n",*ppStab,section->PointerToRawData,section->SizeOfRawData);
        }
    }

    //printf("LOADER: leave find_stab_sections()\n");
}

///////////////////////////////////////////////////////////////////////////////////
// process_pe()
//
///////////////////////////////////////////////////////////////////////////////////
int process_pe(char* filename,int file,void* p,int len)
{

	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_NT_HEADERS pNTHeaders;

	char* pStr;
	PSTAB_ENTRY pStab;
	DWORD nStabLen,nSym;
	char* pStrTab;
	char* pSymTab;

	char szSymName[2048];
	HANDLE fileout;
	int nSymStrLen,nStabStrLen;
    int iRetVal = 0;

	pDosHeader = (PIMAGE_DOS_HEADER)p;
	pNTHeaders = (PIMAGE_NT_HEADERS)((DWORD)p + pDosHeader->e_lfanew);

    if ((pDosHeader->e_magic == IMAGE_DOS_SIGNATURE)
       && (pDosHeader->e_lfanew != 0L)
       && (pNTHeaders->Signature == IMAGE_NT_SIGNATURE))
    {
		if( pNTHeaders->FileHeader.PointerToSymbolTable ){

			pSymTab = (char*)((DWORD)p + pNTHeaders->FileHeader.PointerToSymbolTable);
			nSym = pNTHeaders->FileHeader.NumberOfSymbols;
			//string table follows immediately after symbol table. first 4 bytes give the length of the table
			//references to string table include the first 4 bytes.
			pStrTab = (char*)((PIMAGE_SYMBOL)pSymTab + nSym);
			nSymStrLen = *((DWORD*)pStrTab);
			find_stab_sections(p,IMAGE_FIRST_SECTION(pNTHeaders),pNTHeaders->FileHeader.NumberOfSections,
					&pStab,&nStabLen,&pStr,&nStabStrLen);

			if(pStab && nStabLen && pStr && nStabStrLen)
			{
				LPSTR pDot;

				strcpy(szSymName,filename);
				//printf("LOADER: file name = %s\n",szSymName);
				if((pDot = strchr(szSymName,'.')))
				{
					*pDot = 0;
					strcat(pDot,".dbg");
				}
				else
				{
					strcat(szSymName,".dbg");
				}
				//printf("LOADER: symbol file name = %s\n",szSymName);
	            printf("LOADER: creating symbol file %s for %s\n",szSymName,filename);

				fileout = CreateFile(szSymName,
								     GENERIC_READ | GENERIC_WRITE,
								     0,
								     NULL,
								     CREATE_ALWAYS,
								     0,
								     0);

	            if(fileout != INVALID_HANDLE_VALUE)
				{
					printf("NumberOfSections: %d, size: %d\n", pNTHeaders->FileHeader.NumberOfSections,sizeof(IMAGE_SECTION_HEADER));
					process_stabs(szSymName,
								  fileout,
								  IMAGE_FIRST_SECTION(pNTHeaders),
								  pNTHeaders->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER),
								  p,
								  pStab,
								  nStabLen,
								  pStr,
								  nStabStrLen,
								  (char*)pSymTab,
								  nSym*sizeof(IMAGE_SYMBOL),
								  pStrTab,
								  nSymStrLen);

					CloseHandle(fileout);
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
		else{
            printf("LOADER: file %s does not have a symbol table\n",filename);
            iRetVal = 2;
		}
    }
    else
    {
        printf("LOADER: file %s is not an ELF binary\n",filename);
        iRetVal = 1;
    }

    //printf("LOADER: leave process_pe()\n");
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
    file = _open(filename,O_RDONLY|_O_BINARY);
    if(file>0)
    {
        //printf("LOADER: opened %s as FD %x\n",filename,file);

        len = _lseek(file,0,SEEK_END);
        printf("LOADER: file %s is %u bytes\n",filename,(int)len);

        _lseek(file,0,SEEK_SET);

        p = malloc(len+16);
        if(p)
        {
			long count;
            //printf("LOADER: malloc'd @ %x\n",p);
            memset(p,0,len+16);
            if(len == (count = _read(file,p,len)))
            {
                //printf("LOADER: trying ELF format\n");
                iRetVal = process_pe(filename,file,p,len);
            }
        }
        _close(file);
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
HANDLE	open_debugger(void)
{
    debugger_file = CreateFile("\\Device\\Pice",GENERIC_READ,0,NULL,OPEN_EXISTING,NULL,NULL);
	if(debugger_file == INVALID_HANDLE_VALUE)
	{
		printf("LOADER: debugger is not loaded. Last Error: %ld\n", GetLastError());
	}

	return debugger_file;
}

///////////////////////////////////////////////////////////////////////////////////
// close_debugger()
//
///////////////////////////////////////////////////////////////////////////////////
void close_debugger(void)
{
	if( !CloseHandle(debugger_file) ){
		printf("Error closing debugger handle: %ld\n", GetLastError());
	}
}

int ioctl( HANDLE device, DWORD ioctrlcode, PDEBUGGER_STATUS_BLOCK psb)
{
	 DEBUGGER_STATUS_BLOCK tsb;
	 DWORD bytesreturned;
	 if( !DeviceIoControl( device, ioctrlcode, psb, sizeof(DEBUGGER_STATUS_BLOCK),
			&tsb, sizeof(DEBUGGER_STATUS_BLOCK),&bytesreturned, NULL) ){
		printf("Error in DeviceIoControl: %ld\n", GetLastError());
		return -EINVAL;
	 }
	 else{
		memcpy( psb, &tsb, sizeof(DEBUGGER_STATUS_BLOCK) );
	 }
	 return 0;
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
	DEBUGGER_STATUS_BLOCK sb;

	strcpy(sb.filename, pfilename);

	switch(action)
	{
		case ACTION_LOAD:
			printf("LOADER: loading symbols from %s\n",pfilename);
			if(open_debugger() != INVALID_HANDLE_VALUE)
			{
				iRetVal = ioctl(debugger_file,PICE_IOCTL_LOAD,&sb);
				close_debugger();
			}
			break;
		case ACTION_UNLOAD:
			printf("LOADER: unloading symbols from %s\n",pfilename);
			if(open_debugger() != INVALID_HANDLE_VALUE)
			{
				iRetVal = ioctl(debugger_file,PICE_IOCTL_UNLOAD,&sb);
				close_debugger();
			}
			break;
		case ACTION_RELOAD:
			printf("LOADER: reloading all symbols\n");
			if(open_debugger() != INVALID_HANDLE_VALUE)
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

// Dynamic install to be added later
#if 0
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

    if( open_debugger() != INVALID_HANDLE_VALUE  )
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
    if(open_debugger() == INVALID_HANDLE_VALUE)
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
#endif

///////////////////////////////////////////////////////////////////////////////////
// showstatus()
//
///////////////////////////////////////////////////////////////////////////////////
void showstatus(void)
{
    DEBUGGER_STATUS_BLOCK sb;
    int iRetVal;

	if(open_debugger() != INVALID_HANDLE_VALUE)
	{
		iRetVal = ioctl(debugger_file,PICE_IOCTL_STATUS,&sb);

        //printf("LOADER: Test = %X\n",sb.Test);
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

	if(open_debugger() != INVALID_HANDLE_VALUE)
	{
		iRetVal = ioctl(debugger_file,PICE_IOCTL_BREAK,NULL);
		close_debugger();
	}
}

///////////////////////////////////////////////////////////////////////////////////
// doterminal()
//
///////////////////////////////////////////////////////////////////////////////////
#if 0
void doterminal(void)
{
    if(SetupSerial(2,B115200))
    {
        DebuggerShell();
        CloseSerial();
    }
}
#endif

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
#if 0
        case ACTION_UNINSTALL:
            close_debugger();
            tryuninstall();
            break;
        case ACTION_INSTALL:
            tryinstall();
            break;
#endif
        case ACTION_STATUS:
            showstatus();
            break;
        case ACTION_BREAK:
            dobreak();
            break;
#if 0
        case ACTION_TERMINAL:
            doterminal();
            break;
#endif
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
    if(argc==1 || argc>3)
    {
		showhelp();

		return 1;
    }

	return process_switches(argc,argv);
}
