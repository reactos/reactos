/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module ModuleName:

    symbols.c

Abstract:

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    19-Aug-1998:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"
#include <linux/elf.h>
#include "stab_gnu.h"

#include <linux/vmalloc.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/ctype.h>

PVOID pExports=0;
ULONG ulExportLen=0;

LOCAL_VARIABLE local_vars[512];

PICE_SYMBOLFILE_HEADER* apSymbols[32]={NULL,};
ULONG ulNumSymbolsLoaded=0;

ULONG kernel_end=0;

// artificial module structure if address is in kernel
struct module fake_kernel_module;

char tempSym[1024]; // temp buffer for output

typedef struct _VRET
{
	ULONG value;
	ULONG type;
	ULONG father_type;
	ULONG error;
	ULONG file;
    ULONG size;
    ULONG address;
    char name[256];
    char type_name[256];
    BOOLEAN bPtrType;
    BOOLEAN bStructType;
    BOOLEAN bArrayType;
	PICE_SYMBOLFILE_HEADER* pSymbols;
}VRET,*PVRET;

ULONG ulIndex;
LPSTR pExpression;
VRET vr;
VRET vrStructMembers[1024];
ULONG ulNumStructMembers;

BOOLEAN Expression(PVRET pvr);

//************************************************************************* 
// InitFakeKernelModule() 
// 
//************************************************************************* 
BOOLEAN InitFakeKernelModule(void)
{
    struct module* pMod;

    ENTER_FUNC();
    
    if(pmodule_list)
    {

	    DPRINT((0,"InitFakeKernelModule(): *pmodule_list = %x\n",(ULONG)*pmodule_list));
		if(IsAddressValid((ULONG)*pmodule_list) )
		{
			pMod = *pmodule_list;
			DPRINT((0,"InitFakeKernelModule(): start pMod = %x\n",(ULONG)pMod));
			do
			{
				if(!pMod->size)
				{
					DPRINT((0,"InitFakeKernelModule(): pMod = %x\n",(ULONG)pMod));
					fake_kernel_module = * pMod;
					PICE_strcpy((LPSTR)(fake_kernel_module.name),"vmlinux");
					fake_kernel_module.size = kernel_end - KERNEL_START;
				    DPRINT((0,"InitFakeKernelModule(): SUCCESS\n"));
                    LEAVE_FUNC();
					return TRUE;
				}
			}while((pMod = pMod->next));
		}
    }
    LEAVE_FUNC();
	return FALSE;
}

//************************************************************************* 
// ScanExports() 
// 
//************************************************************************* 
BOOLEAN ScanExports(const char *pFind,PULONG pValue)
{
	char temp[256];
	LPSTR pStr=NULL;
	LPSTR pExp = pExports;
	BOOLEAN bResult = FALSE;

	ENTER_FUNC();

nomatch:
	if(pExports)
		pStr = strstr(pExp,pFind);

	if(pStr)
	{
		LPSTR p;
		ULONG state;
		LPSTR pOldStr = pStr;

		for(;*pStr!=0x0a && (ULONG)pStr>=(ULONG)pExports;pStr--);
		pStr++;
		p = temp;
		for(;*pStr!=0x0a;)*p++=*pStr++;
		*p=0;
		p = PICE_strtok(temp," ");
		state=0;
		while(p)
		{
			switch(state)
			{
				case 0:
					ConvertTokenToHex(p,pValue);
					break;
				case 1:
					break;
				case 2:
					if(PICE_strcmp(p,pFind)!=0)
					{
						pExp = pOldStr+1;	
						goto nomatch;
					}
					state = -1;
					bResult = TRUE;
		            DPRINT((0,"%s @ %x\n",pFind,*pValue));
					break;
						
			}
			state++;
			p = PICE_strtok(NULL," ");
		}
	}

	LEAVE_FUNC();

	return bResult;
}

//************************************************************************* 
// ReadHex() 
// 
//************************************************************************* 
BOOLEAN ReadHex(LPSTR p,PULONG pValue)
{
    ULONG result=0,i;

	for(i=0;i<8 && p[i]!=0 && p[i]!=' ';i++)
	{
		if(p[i]>='0' && p[i]<='9')
		{
			result<<=4;
			result|=(ULONG)(UCHAR)(p[i]-'0');
		}
		else if(p[i]>='A' && p[i]<='F')
		{
			result<<=4;
			result|=(ULONG)(UCHAR)(p[i]-'A'+10);
		}
		else if(p[i]>='a' && p[i]<='f')
		{
			result<<=4;
			result|=(ULONG)(UCHAR)(p[i]-'a'+10);
		}
		else
			return FALSE;
	}

    *pValue = result;
    return TRUE;
}

//************************************************************************* 
// ScanExportLine() 
// 
//************************************************************************* 
BOOLEAN ScanExportLine(LPSTR p,PULONG ulValue,LPSTR* ppPtrToSymbol)
{
    BOOLEAN bResult = FALSE;

    if(ReadHex(p,ulValue))
    {
        p += 11;
        *ppPtrToSymbol += 11;
        bResult = TRUE;
    }

    return bResult;
}

//************************************************************************* 
// ValidityCheckSymbols() 
// 
//************************************************************************* 
BOOLEAN ValidityCheckSymbols(PICE_SYMBOLFILE_HEADER* pSymbols)
{
	BOOLEAN bRet;

    DPRINT((0,"ValidityCheckSymbols()\n"));

	bRet = (((ULONG)pSymbols>=TASK_SIZE) &&
			IsRangeValid((ULONG)pSymbols + pSymbols->ulOffsetToHeaders,pSymbols->ulSizeOfHeader) &&
		    IsRangeValid((ULONG)pSymbols + pSymbols->ulOffsetToGlobals,pSymbols->ulSizeOfGlobals) &&
		    IsRangeValid((ULONG)pSymbols + pSymbols->ulOffsetToGlobalsStrings,pSymbols->ulSizeOfGlobalsStrings) &&
		    IsRangeValid((ULONG)pSymbols + pSymbols->ulOffsetToStabs,pSymbols->ulSizeOfStabs) &&
		    IsRangeValid((ULONG)pSymbols + pSymbols->ulOffsetToStabsStrings,pSymbols->ulSizeOfStabsStrings) );

    DPRINT((0,"ValidityCheckSymbols(): symbols are %s\n",bRet?"VALID":"NOT VALID"));
			
	return bRet;
}

//************************************************************************* 
// FindModuleSymbols() 
// 
//************************************************************************* 
PICE_SYMBOLFILE_HEADER* FindModuleSymbols(ULONG addr)
{
    struct module* pMod;
    ULONG start,end,i;

    DPRINT((0,"FindModuleSymbols(%x)\n",addr));
    if(pmodule_list)
    {
        i=0;
        pMod = *pmodule_list;
        do
        {
            if(pMod->size)
			{
                start = (ULONG)pMod+sizeof(struct module);
                end = start + pMod->size-sizeof(struct module);
                DPRINT((0,"FindModuleSymbols(): %s %x-%x\n",pMod->name,start,end));
                if(addr>=start && addr<end)
                {
                    DPRINT((0,"FindModuleSymbols(): address matches %s %x-%x\n",pMod->name,start,end));
                    for(i=0;i<ulNumSymbolsLoaded;i++)
                    {
                        if(PICE_strcmpi((LPSTR)pMod->name,apSymbols[i]->name) == 0)
						{
							if(ValidityCheckSymbols(apSymbols[i]))
	                            return apSymbols[i];
							else
								return NULL;
						}
                    }
                }
            }
            else
            {
                start = (ULONG)KERNEL_START + sizeof(struct module);
                end = start + fake_kernel_module.size-sizeof(struct module);
                DPRINT((0,"FindModuleSymbols(): %s %x-%x\n",fake_kernel_module.name,start,end));
                if(addr>=start && addr<end)
                {
                    DPRINT((0,"FindModuleSymbols(): address matches %s %x-%x\n",fake_kernel_module.name,start,end));
                    for(i=0;i<ulNumSymbolsLoaded;i++)
                    {
                        if(PICE_strcmpi((LPSTR)fake_kernel_module.name,apSymbols[i]->name) == 0)
						{
							if(ValidityCheckSymbols(apSymbols[i]))
	                            return apSymbols[i];
							else
								return NULL;
						}
                    }
                }
            }
        }while((pMod = pMod->next));
    }

    return NULL;
}

//************************************************************************* 
// FindModuleFromAddress() 
// 
//************************************************************************* 
struct module* FindModuleFromAddress(ULONG addr)
{
    struct module* pMod;
    ULONG start,end;

    DPRINT((0,"FindModuleFromAddress()\n"));
    if(pmodule_list)
    {
        pMod = *pmodule_list;
        do
        {
			if(pMod->size)
			{
                start = (ULONG)pMod+sizeof(struct module);
                end = start + pMod->size-sizeof(struct module);
                DPRINT((0,"FindModuleFromAddress(): %s %x-%x\n",pMod->name,start,end));
                if(addr>=start && addr<end)
                {
                    DPRINT((0,"FindModuleFromAddress(): found %s\n",pMod->name));
                    return pMod;
                }
            }
            // must be the kernel
            else
            {
                start = (ULONG)KERNEL_START + sizeof(struct module);
                end = start + fake_kernel_module.size-sizeof(struct module);
                DPRINT((0,"FindModuleFromAddress(): %s %x-%x\n",fake_kernel_module.name,start,end));
                if(addr>=start && addr<end)
                {
                    DPRINT((0,"FindModuleFromAddress(): found %s\n",fake_kernel_module.name));
                    return &fake_kernel_module;
                }
            }
        }while((pMod = pMod->next));
    }

    return NULL;
}

//************************************************************************* 
// FindModuleByName() 
// 
//************************************************************************* 
struct module* FindModuleByName(LPSTR modname)
{
    struct module* pMod;

    DPRINT((0,"FindModuleFromAddress()\n"));
    if(pmodule_list)
    {
        pMod = *pmodule_list;
        do
        {
			if(pMod->size)
			{
				if(PICE_strcmpi(modname,(LPSTR)pMod->name) == 0)
                {
                    DPRINT((0,"FindModuleByName(): found %s\n",pMod->name));
                    return pMod;
                }
            }
			else
			{
				if(PICE_strcmpi(modname,(LPSTR)fake_kernel_module.name) == 0)
                {
                    DPRINT((0,"FindModuleByName(): found %s\n",fake_kernel_module.name));
                    return &fake_kernel_module;
                }
            }
        }while((pMod = pMod->next));
    }

    return NULL;
}

//************************************************************************* 
// FindModuleSymbolsByModuleName() 
// 
//************************************************************************* 
PICE_SYMBOLFILE_HEADER* FindModuleSymbolsByModuleName(LPSTR modname)
{
    ULONG i;

    DPRINT((0,"FindModuleSymbols()\n"));
    for(i=0;i<ulNumSymbolsLoaded;i++)
    {
        if(PICE_strcmpi(modname,apSymbols[i]->name) == 0)
            return apSymbols[i];
    }

    return NULL;
}

//************************************************************************* 
// ScanExportsByAddress() 
// 
//************************************************************************* 
BOOLEAN ScanExportsByAddress(LPSTR *pFind,ULONG ulValue)
{
	char temp[256];
    static char temp3[256];
    LPSTR p,pStartOfLine,pSymbolName=NULL;
    ULONG ulCurrentValue=0,i;
    BOOLEAN bResult = FALSE;
	struct module *pMod;
    ULONG ulMinValue = -1;
	Elf32_Sym* pElfSym;
	LPSTR pElfStr;
	Elf32_Shdr* pElfShdr;
    PICE_SYMBOLFILE_HEADER* pSymbols;

	ENTER_FUNC();

    if(ulValue < TASK_SIZE)
    {
    	LEAVE_FUNC();
        return FALSE;
    }

    pSymbols = FindModuleSymbols(ulValue);
	if(pSymbols && pmodule_list)
	{
        struct module* pModTemp;

		DPRINT((0,"looking up symbols\n"));
        pMod = *pmodule_list;
        do
        {
            if(!pMod->size)
                pModTemp = &fake_kernel_module;
            else
                pModTemp = pMod;

			if(ulValue>=((ULONG)pModTemp+sizeof(struct module)) && ulValue<((ULONG)pModTemp+pModTemp->size-sizeof(struct module)))
			{
				if(PICE_strcmpi((LPSTR)pModTemp->name,pSymbols->name) == 0)
				{
					DPRINT((0,"ScanExportsByAddress(): found symbols for module %s @ \n",pModTemp->name,(ULONG)pSymbols));

					pElfSym = (Elf32_Sym*)((ULONG)pSymbols+pSymbols->ulOffsetToGlobals);
					pElfStr = (LPSTR)((ULONG)pSymbols+pSymbols->ulOffsetToGlobalsStrings);
					pElfShdr = (Elf32_Shdr*)((ULONG)pSymbols+pSymbols->ulOffsetToHeaders);

					DPRINT((0,"ScanExportsByAddress(): pElfSym = %x\n",pElfSym));
					DPRINT((0,"ScanExportsByAddress(): pElfStr = %x\n",pElfStr));
					DPRINT((0,"ScanExportsByAddress(): pElfShdr = %x\n",pElfShdr));

					DPRINT((0,"ScanExportsByAddress(): %s has %u symbols\n",pSymbols->name,pSymbols->ulSizeOfGlobals/sizeof(Elf32_Sym)));

					for(i=0;i<(pSymbols->ulSizeOfGlobals/sizeof(Elf32_Sym));i++)
					{
						if((ELF32_ST_BIND(pElfSym->st_info)==STB_GLOBAL || ELF32_ST_BIND(pElfSym->st_info)==STB_LOCAL || ELF32_ST_BIND(pElfSym->st_info)==STB_WEAK)  &&
						   (ELF32_ST_TYPE(pElfSym->st_info)==STT_OBJECT || ELF32_ST_TYPE(pElfSym->st_info)==STT_FUNC) && 
						   (pElfSym->st_shndx<SHN_LORESERVE || pElfSym->st_shndx==SHN_ABS || pElfSym->st_shndx==SHN_COMMON))
						{
							LPSTR pName = &pElfStr[pElfSym->st_name];
							ULONG start,end;

							DPRINT((0,"ScanExportsByAddress(): pName = %x\n",(ULONG)pName));

							if(!IsAddressValid((ULONG)pName) )
							{
								DPRINT((0,"ScanExportsByAddress(): pName is not a valid pointer\n"));
								return FALSE;
							}

							DPRINT((0,"ScanExportsByAddress(): pName = %s\n",pName));

							if(!IsRangeValid((ULONG)pElfSym,sizeof(Elf32_Sym) ) )
							{
								DPRINT((0,"ScanExportsByAddress(): pElfSym = %x is not a valid pointer\n",(ULONG)pElfSym));
								return FALSE;
							}

							DPRINT((0,"ScanExportsByAddress(): pModTemp = %x\n",(ULONG)pModTemp));
                            if(pModTemp != &fake_kernel_module)
                            {
								Elf32_Shdr* pElfShdrThis = (Elf32_Shdr*)pElfShdr + pElfSym->st_shndx;

								DPRINT((0,"ScanExportsByAddress(): module is not kernel\n"));

								DPRINT((0,"ScanExportsByAddress(): pElfShdr[%x] = %x\n",pElfSym->st_shndx,(ULONG)pElfShdrThis));

								if(!IsRangeValid((ULONG)pElfShdrThis,sizeof(Elf32_Shdr)) )
								{
									DPRINT((0,"ScanExportsByAddress(): pElfShdr[%x] = %x is not a valid pointer\n",pElfSym->st_shndx,pElfShdrThis));
									return FALSE;
								}

							    start = ((ULONG)pModTemp+pElfShdrThis->sh_offset);
								DPRINT((0,"ScanExportsByAddress(): start [1] = %x\n",start));

							    start = (start+pElfShdrThis->sh_addralign)&~(pElfShdrThis->sh_addralign-1);
								DPRINT((0,"ScanExportsByAddress(): start [2] = %x\n",start));

							    start += pElfSym->st_value;
								DPRINT((0,"ScanExportsByAddress(): start [3] = %x\n",start));
                            }
                            else
                            {
								DPRINT((0,"ScanExportsByAddress(): module is kernel\n"));
                                start = pElfSym->st_value;
								DPRINT((0,"ScanExportsByAddress(): start [1] = %x\n",start));
                            }

							end = start+pElfSym->st_size;
							DPRINT((0,"ScanExportsByAddress(): end = %x\n",end));

							if(ulValue>=start && ulValue<end)
							{
							    DPRINT((0,"[%u] %.8X %.8X %.8X %.8X %.8X %.8X %.8X\n",
										pElfSym->st_shndx,
										((ULONG)pModTemp+pElfShdr[pElfSym->st_shndx].sh_offset),
										pElfShdr[pElfSym->st_shndx].sh_addr,
										pElfShdr[pElfSym->st_shndx].sh_offset,
										pElfShdr[pElfSym->st_shndx].sh_size,
										pElfShdr[pElfSym->st_shndx].sh_type,
										pElfShdr[pElfSym->st_shndx].sh_link,
										pElfShdr[pElfSym->st_shndx].sh_addralign));
								DPRINT((0,"in section [%u] %8x value = %x module struct %x (%x)\n",pElfSym->st_shndx,pElfShdr[pElfSym->st_shndx].sh_offset,ulValue,sizeof(struct module),((sizeof(struct module)+0x10)&~0x0F)));
								DPRINT((0,"[%u] %32s %.8X %.8X %.8X %.8X %.8X %.8X\n",i,pName,pElfSym->st_name,pElfSym->st_value,pElfSym->st_info,pElfSym->st_other,pElfSym->st_size,pElfSym->st_shndx));
								DPRINT((0,"start %x end %x\n",start,end));
								*pFind = temp3;
								if(ulValue-start)
									PICE_sprintf(temp3,"%s!%s+%x",pModTemp->name,pName,ulValue-start);
								else
									PICE_sprintf(temp3,"%s!%s",pModTemp->name,pName);
								return TRUE;
							}

						}
						pElfSym++;
					}
				}
			}
        }while((pMod = pMod->next));
	}

    if(pExports && ulValue >= TASK_SIZE && ulValue < kernel_end)
    {
        p = pExports;
        // while we bound in System.map
        while(p<((LPSTR)pExports+ulExportLen))
        {
            // make a temp ptr to the line we can change
            pStartOfLine = p;
            // will read the hex value and return a pointer to the symbol name
            if(ScanExportLine(p,&ulCurrentValue,&pStartOfLine))
            {
                if(ulValue>=ulCurrentValue && (ulValue-ulCurrentValue)<ulMinValue)
                {
                    // save away our info for later
                    ulMinValue = ulValue-ulCurrentValue;
                    pSymbolName = pStartOfLine;
                    bResult = TRUE;
                    *pFind = temp3;
					if(ulMinValue==0)
						break;
                }
            }
            // increment pointer to next line
            p = pStartOfLine;
            while(*p!=0 && *p!=0x0a)p++;
                p++;
        }
        if(bResult)
        {
            // copy symbol name to temp string
            for(i=0;pSymbolName[i]!=0 && pSymbolName[i]!=0x0a;i++)
                temp[i] = pSymbolName[i];
            temp[i] = 0;
            // decide if we need to append an offset
            if(ulMinValue)
                PICE_sprintf(temp3,"vmlinux!%s+%.8X",temp,ulMinValue);
            else
                PICE_sprintf(temp3,"vmlinux!%s",temp);
        }
    }

    if(pmodule_list && ulMinValue!=0)
    {
        pMod = *pmodule_list;
        do
        {
			if(ulValue>=((ULONG)pMod+sizeof(struct module)) && ulValue<((ULONG)pMod+pMod->size-sizeof(struct module)))
            {
			    if(pMod->syms)
			    {
				    for(i=0;i<pMod->nsyms;i++)
				    {
					    ulCurrentValue = pMod->syms[i].value;
					    if(ulValue>=ulCurrentValue && (LONG)(ulValue-ulCurrentValue)<ulMinValue)
					    {
						    ulMinValue = ulValue-ulCurrentValue;

						    if(ulValue-ulCurrentValue)
							    PICE_sprintf(temp3,"%s!%s+%.8X",pMod->name,pMod->syms[i].name,ulValue-ulCurrentValue);
						    else
							    PICE_sprintf(temp3,"%s!%s",pMod->name,pMod->syms[i].name);
						    bResult = TRUE;
						    *pFind = temp3;
						    if(ulMinValue == 0)
							    break;
					    }
				    }
			    }

			    // this could be near entry and cleanup of a module
			    ulCurrentValue = (ULONG)pMod->init;
                if(ulCurrentValue)
                {
			        if(ulValue>=ulCurrentValue && (LONG)(ulValue-ulCurrentValue)<ulMinValue)
			        {
				        ulMinValue = ulValue-ulCurrentValue;

				        if(ulValue-ulCurrentValue)
					        PICE_sprintf(temp3,"%s!init_module+%.8X",pMod->name,ulValue-ulCurrentValue);
				        else
					        PICE_sprintf(temp3,"%s!init_module",pMod->name);
				        bResult = TRUE;
				        *pFind = temp3;
				        if(ulMinValue == 0)
					        break;
			        }
                }

                ulCurrentValue = (ULONG)pMod->cleanup;
                if(ulCurrentValue)
                {
			        if(ulValue>=ulCurrentValue && (LONG)(ulValue-ulCurrentValue)<ulMinValue)
			        {
				        ulMinValue = ulValue-ulCurrentValue;

				        if(ulValue-ulCurrentValue)
					        PICE_sprintf(temp3,"%s!cleanup_module+%.8X",pMod->name,ulValue-ulCurrentValue);
				        else
					        PICE_sprintf(temp3,"%s!cleanup_module",pMod->name);
				        bResult = TRUE;
				        *pFind = temp3;
				        if(ulMinValue == 0)
					        break;
			        }
                }
            }
        }while((pMod = pMod->next));
    }

	LEAVE_FUNC();

	return bResult;
}

//************************************************************************* 
// FindFunctionByAddress() 
// 
//************************************************************************* 
LPSTR FindFunctionByAddress(ULONG ulValue,PULONG pulstart,PULONG pulend)
{
	Elf32_Sym* pElfSym;
	LPSTR pElfStr;
	Elf32_Shdr* pElfShdr;
	struct module *pMod;
	struct module *pModTemp;
	int i;
    PICE_SYMBOLFILE_HEADER* pSymbols;
    ULONG start_mod,end_mod;

    pSymbols = FindModuleSymbols(ulValue);
    DPRINT((0,"FindFunctionByAddress(): symbols @ %x\n",(ULONG)pSymbols));
	if(pSymbols && pmodule_list)
	{
		DPRINT((0,"looking up symbol\n"));
        pMod = *pmodule_list;
        do
        {
			if(pMod->size)
            {
                pModTemp = pMod;
                start_mod = (ULONG)pModTemp+sizeof(struct module);
                end_mod = (ULONG)pModTemp+pModTemp->size;
            }
            else
            {
                pModTemp = &fake_kernel_module;
                start_mod = (ULONG)KERNEL_START+sizeof(struct module);
                end_mod = (ULONG)KERNEL_START+pModTemp->size;
            }

            DPRINT((0,"FindFunctionByAddress(): is it %s for %x\n",(ULONG)pModTemp->name,ulValue));

			if(ulValue>=start_mod && ulValue<end_mod)
			{
                DPRINT((0,"FindFunctionByAddress(): address matches %s\n",(ULONG)pModTemp->name));
				if(PICE_strcmpi((LPSTR)pModTemp->name,pSymbols->name) == 0)
				{
					DPRINT((0,"found symbols for module %s\n",pModTemp->name));
					pElfSym = (Elf32_Sym*)((ULONG)pSymbols+pSymbols->ulOffsetToGlobals);
					pElfStr = (LPSTR)((ULONG)pSymbols+pSymbols->ulOffsetToGlobalsStrings);
					pElfShdr = (Elf32_Shdr*)((ULONG)pSymbols+pSymbols->ulOffsetToHeaders);
					DPRINT((0,"pElfSym = %x\n",pElfSym));
					DPRINT((0,"pElfStr = %x\n",pElfStr));
					DPRINT((0,"pElfShdr = %x\n",pElfShdr));
					for(i=0;i<(pSymbols->ulSizeOfGlobals/sizeof(Elf32_Sym));i++)
					{
						if((ELF32_ST_BIND(pElfSym->st_info)==STB_GLOBAL || ELF32_ST_BIND(pElfSym->st_info)==STB_LOCAL || ELF32_ST_BIND(pElfSym->st_info)==STB_LOCAL)&&
						   ELF32_ST_TYPE(pElfSym->st_info)==STT_FUNC && 
						   (pElfSym->st_shndx<SHN_LORESERVE || pElfSym->st_shndx==SHN_ABS || pElfSym->st_shndx==SHN_COMMON))
						{
							LPSTR pName = &pElfStr[pElfSym->st_name];
							ULONG start,end;

						    if(pMod->size)
                            {
                                start = ((ULONG)pModTemp+pElfShdr[pElfSym->st_shndx].sh_offset);
							    start = (start+pElfShdr[pElfSym->st_shndx].sh_addralign)&~(pElfShdr[pElfSym->st_shndx].sh_addralign-1);

							    start += pElfSym->st_value;
                            }
                            else
                            {
                                start = pElfSym->st_value;
                            }

							end = start+pElfSym->st_size;

							DPRINT((0,"[%u] %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X\n",
                                    i,
                                    start,
                                    end,
									pElfSym->st_shndx,
									pElfShdr[pElfSym->st_shndx].sh_addr,
									pElfShdr[pElfSym->st_shndx].sh_offset,
									pElfShdr[pElfSym->st_shndx].sh_size,
									pElfShdr[pElfSym->st_shndx].sh_type,
									pElfShdr[pElfSym->st_shndx].sh_link,
									pElfShdr[pElfSym->st_shndx].sh_addralign));

                            if(ulValue>=start && ulValue<end)
							{
							    DPRINT((0,"[%u] %.8X %.8X %.8X %.8X %.8X %.8X %.8X\n",
										pElfSym->st_shndx,
										((ULONG)pModTemp+pElfShdr[pElfSym->st_shndx].sh_offset),
										pElfShdr[pElfSym->st_shndx].sh_addr,
										pElfShdr[pElfSym->st_shndx].sh_offset,
										pElfShdr[pElfSym->st_shndx].sh_size,
										pElfShdr[pElfSym->st_shndx].sh_type,
										pElfShdr[pElfSym->st_shndx].sh_link,
										pElfShdr[pElfSym->st_shndx].sh_addralign));
								DPRINT((0,"in section [%u] %8x value = %x module struct %x (%x)\n",pElfSym->st_shndx,pElfShdr[pElfSym->st_shndx].sh_offset,ulValue,sizeof(struct module),((sizeof(struct module)+0x10)&~0x0F)));
								DPRINT((0,"[%u] %32s %.8X %.8X %.8X %.8X %.8X %.8X\n",i,pName,pElfSym->st_name,pElfSym->st_value,pElfSym->st_info,pElfSym->st_other,pElfSym->st_size,pElfSym->st_shndx));
								DPRINT((0,"start %x end %x\n",start,end));
                            
                                if(pulstart)
                                    *pulstart = start;
                                if(pulend)
                                    *pulend = end;
								return pName;
							}

						}
						pElfSym++;
					}
				}
			}
        }while((pMod = pMod->next));
	}

	return NULL;
}

//************************************************************************* 
// FindDataSectionOffset() 
// 
//************************************************************************* 
ULONG FindDataSectionOffset(Elf32_Shdr* pSHdr)
{

    DPRINT((0,"FindDataSectionOffset()\n"));

    while(1)
    {
        DPRINT((0,"FindDataSectionOffset(): sh_offset %.8X sh_addr = %.8X\n",pSHdr->sh_offset,pSHdr->sh_addr));
        if((pSHdr->sh_flags & (SHF_WRITE|SHF_ALLOC)	) == (SHF_WRITE|SHF_ALLOC))
        {

            return pSHdr->sh_offset;
        }
        pSHdr++;
    }

    return 0;
}

//************************************************************************* 
// FindFunctionInModuleByNameViaKsyms() 
// 
//************************************************************************* 
ULONG FindFunctionInModuleByNameViaKsyms(struct module* pMod,LPSTR szFunctionname)
{
    ULONG i;

    ENTER_FUNC();

    if(pMod->nsyms)
    {
        DPRINT((0,"FindFunctionInModuleByNameViaKsyms(): %u symbols for module %s\n",pMod->nsyms,pMod->name));
        for(i=0;i<pMod->nsyms;i++)
        {
            DPRINT((0,"FindFunctionInModuleByNameViaKsyms(): %s\n",pMod->syms[i].name));
            if(PICE_strcmpi((LPSTR)pMod->syms[i].name,szFunctionname) == 0)
            {
                DPRINT((0,"FindFunctionInModuleByName(): symbol was in exports\n"));
                LEAVE_FUNC();
                return pMod->syms[i].value;
            }
        }
    }

    DPRINT((0,"FindFunctionInModuleByName(): symbol wasn't in exports\n"));
    LEAVE_FUNC();
    return 0;
}

//************************************************************************* 
// FindFunctionInModuleByName() 
// 
//************************************************************************* 
ULONG FindFunctionInModuleByName(LPSTR szFunctionname,struct module* pMod)
{
    ULONG i,addr;
    PICE_SYMBOLFILE_HEADER* pSymbols=NULL;
	Elf32_Sym* pElfSym;
	LPSTR pElfStr;
	Elf32_Shdr* pElfShdr;

    ENTER_FUNC();
    DPRINT((0,"FindFunctionInModuleByName(%s)\n",szFunctionname));

    DPRINT((0,"FindFunctionInModuleByName(): mod size = %x\n",pMod->size));
    if(pMod == &fake_kernel_module)
    {
        DPRINT((0,"FindFunctionInModuleByName(): module is kernel\n"));
        addr = KERNEL_START;
    }
    else
    {
        DPRINT((0,"FindFunctionInModuleByName(): module is %s\n",pMod->name));
        addr = (ULONG)pMod;
    }

    addr += sizeof(struct module);

    pSymbols = FindModuleSymbols(addr);
    if(pSymbols)
    {
        DPRINT((0,"FindFunctionInModuleByName(): found symbol table for %s\n",pSymbols->name));
		pElfSym = (Elf32_Sym*)((ULONG)pSymbols+pSymbols->ulOffsetToGlobals);
		pElfStr = (LPSTR)((ULONG)pSymbols+pSymbols->ulOffsetToGlobalsStrings);
        pElfShdr = (Elf32_Shdr*)((ULONG)pSymbols+pSymbols->ulOffsetToHeaders);

		for(i=0;i<(pSymbols->ulSizeOfGlobals/sizeof(Elf32_Sym));i++)
		{
            DPRINT((0,"FindFunctionInModuleByName(): BIND = %.8X TYPE = %.8X\n",ELF32_ST_BIND(pElfSym->st_info),ELF32_ST_TYPE(pElfSym->st_info)));

			if((ELF32_ST_BIND(pElfSym->st_info)==STB_GLOBAL || ELF32_ST_BIND(pElfSym->st_info)==STB_LOCAL || ELF32_ST_BIND(pElfSym->st_info)==STB_WEAK) &&
			    (ELF32_ST_TYPE(pElfSym->st_info)==STT_FUNC || ELF32_ST_TYPE(pElfSym->st_info)==STT_OBJECT) && 
			   (pElfSym->st_shndx<SHN_LORESERVE))
			{
                LPSTR pName = &pElfStr[pElfSym->st_name];
                ULONG start;

				DPRINT((0,"[%u] %.8X  %.8X %.8X %.8X %.8X %.8X %.8X %.8X\n",
						pElfSym->st_shndx,
                        pElfSym->st_value,
						((ULONG)pMod+pElfShdr[pElfSym->st_shndx].sh_offset),
						pElfShdr[pElfSym->st_shndx].sh_addr,
						pElfShdr[pElfSym->st_shndx].sh_offset,
						pElfShdr[pElfSym->st_shndx].sh_size,
						pElfShdr[pElfSym->st_shndx].sh_type,
						pElfShdr[pElfSym->st_shndx].sh_link,
						pElfShdr[pElfSym->st_shndx].sh_addralign));

                if(pMod->size && pMod != &fake_kernel_module)
                {
				    start = ((ULONG)pMod+pElfShdr[pElfSym->st_shndx].sh_offset);

				    start = (start+pElfShdr[pElfSym->st_shndx].sh_addralign)&~(pElfShdr[pElfSym->st_shndx].sh_addralign-1);

				    start += pElfSym->st_value;
                }
                else
                {
				    start = pElfSym->st_value;
                }

                DPRINT((0,"FindFunctionInModuleByName(): %s @ %x\n",pName,start));

                if(PICE_strcmpi(pName,szFunctionname) == 0 && start)
                {
                    DPRINT((0,"FindFunctionInModuleByName(): symbol was in symbol table\n"));
                    LEAVE_FUNC();
                    return start;
                }
            }
            pElfSym++;
        }

    }

    // for all symbols in BSS we can't find their location via the external symbols since
    // insmod puts BSS and data anywhere it likes.
    // so we try to look up the symbol via the kernel symbol table
    DPRINT((0,"FindFunctionInModuleByName(): try lookup via kernel symbol table\n"));
    LEAVE_FUNC();
    return FindFunctionInModuleByNameViaKsyms(pMod,szFunctionname);
}

//************************************************************************* 
// ExtractTypeNumber() 
// 
//************************************************************************* 
ULONG ExtractTypeNumber(LPSTR p)
{
	LPSTR pTypeNumber;
	ULONG ulTypeNumber = 0;

    DPRINT((0,"ExtractTypeNumber(%s)\n",p));
	pTypeNumber = PICE_strchr(p,'(');
	if(pTypeNumber)
	{
		pTypeNumber++;
		ulTypeNumber = ExtractNumber(pTypeNumber);
		ulTypeNumber <<= 16;
		pTypeNumber = PICE_strchr(p,',');
        if(pTypeNumber)
        {
		    pTypeNumber++;
		    ulTypeNumber += ExtractNumber(pTypeNumber);
        }
        else
        {
            ulTypeNumber = 0;
        }
	}
	return ulTypeNumber;
}

//************************************************************************* 
// FindTypeDefinitionForCombinedTypes() 
// 
//************************************************************************* 
LPSTR FindTypeDefinitionForCombinedTypes(PICE_SYMBOLFILE_HEADER* pSymbols,ULONG ulTypeNumber,ULONG ulFileNumber)
{
    ULONG i;
    PSTAB_ENTRY pStab;
    LPSTR pStr,pName,pTypeNumber,pTypeDefIncluded,pNameTemp;
    int nStabLen;
    int nOffset=0,nNextOffset=0,nLen;
	static char szAccumulatedName[2048];
	ULONG ulCurrentTypeNumber,ulCurrentFileNumber=0;
    static char szCurrentPath[256];

    ENTER_FUNC();

	*szAccumulatedName = 0;

    pStab = (PSTAB_ENTRY )((ULONG)pSymbols + pSymbols->ulOffsetToStabs);
    nStabLen = pSymbols->ulSizeOfStabs;
    pStr = (LPSTR)((ULONG)pSymbols + pSymbols->ulOffsetToStabsStrings);

    for(i=0;i<(nStabLen/sizeof(STAB_ENTRY));i++)
    {
        pName = &pStr[pStab->n_strx + nOffset];

        switch(pStab->n_type)
        {
            case N_UNDF:
                nOffset += nNextOffset;
                nNextOffset = pStab->n_value;
                break;
            case N_SO:
                if((nLen = PICE_strlen(pName)))
                {
                    if(pName[nLen-1]!='/')
                    {
						ulCurrentFileNumber++;
                        if(PICE_strlen(szCurrentPath))
                        {
                            PICE_strcat(szCurrentPath,pName);
                            DPRINT((0,"FindTypeDefinitionForCombinedTypes(): changing source file %s\n",szCurrentPath));
                        }
                        else
                        {
                            DPRINT((0,"FindTypeDefinitionForCombinedTypes(): changing source file %s\n",pName));
                        }
                    }
                    else
                        PICE_strcpy(szCurrentPath,pName);
                }
                else
				{
                    szCurrentPath[0]=0;
				}
				break;
			case N_GSYM:
                if(ulCurrentFileNumber == ulFileNumber)
                {
                    DPRINT((0,"FindTypeDefinitionForCombinedTypes(): %s\n",pName));

					// handle multi-line symbols
					if(PICE_strchr(pName,'\\'))
					{
						if(PICE_strlen(szAccumulatedName))
						{
							PICE_strcat(szAccumulatedName,pName);
						}
						else
						{
							PICE_strcpy(szAccumulatedName,pName);
						}
                        szAccumulatedName[PICE_strlen(szAccumulatedName)-1]=0;
                        //DPRINT((0,"accum. %s\n",szAccumulatedName));
					}
                    else
                    {
						if(PICE_strlen(szAccumulatedName)==0)
                        {
                            PICE_strcpy(szAccumulatedName,pName);
                        }
                        else
                        {
                            PICE_strcat(szAccumulatedName,pName);
                        }
                        pNameTemp = szAccumulatedName;

                        // symbol-name:type-identifier type-number =
				        nLen = StrLenUpToWhiteChar(pNameTemp,":");
                        if((pTypeDefIncluded = PICE_strchr(pNameTemp,'=')) && pNameTemp[nLen+1]=='G')
                        {
                            DPRINT((0,"FindTypeDefinitionForCombinedTypes(): symbol includes type definition (%s)\n",pNameTemp));
                            pTypeNumber = pNameTemp+nLen+1;
                            if((ulCurrentTypeNumber = ExtractTypeNumber(pTypeNumber)) )
                            {
                                DPRINT((0,"FindTypeDefinitionForCombinedTypes(): type-number %x\n",ulCurrentTypeNumber));
                                if(ulCurrentTypeNumber == ulTypeNumber)
                                {
                                    DPRINT((0,"FindTypeDefinitionForCombinedTypes(): typenumber %x matches!\n",ulCurrentTypeNumber));
                                    return pNameTemp;
                                }
                            }
				        }
                        *szAccumulatedName = 0;
                    }
                }
				break;
        }
        pStab++;
    }
    return NULL;
}

//************************************************************************* 
// FindTypeDefinition() 
// 
//************************************************************************* 
LPSTR FindTypeDefinition(PICE_SYMBOLFILE_HEADER* pSymbols,ULONG ulTypeNumber,ULONG ulFileNumber)
{
    ULONG i;
    PSTAB_ENTRY pStab;
    LPSTR pStr,pName,pTypeString;
    int nStabLen;
    int nOffset=0,nNextOffset=0,strLen;
	static char szAccumulatedName[2048];
	ULONG ulCurrentTypeNumber,ulCurrentFileNumber=0;
	LPSTR pTypeSymbol;
    static char szCurrentPath[256];

    ENTER_FUNC();
    DPRINT((0,"FindTypeDefinition(%u,%u)\n",ulTypeNumber,ulFileNumber));

	*szAccumulatedName = 0;

    pStab = (PSTAB_ENTRY )((ULONG)pSymbols + pSymbols->ulOffsetToStabs);
    nStabLen = pSymbols->ulSizeOfStabs;
    pStr = (LPSTR)((ULONG)pSymbols + pSymbols->ulOffsetToStabsStrings);

    for(i=0;i<(nStabLen/sizeof(STAB_ENTRY));i++)
    {
        pName = &pStr[pStab->n_strx + nOffset];

        switch(pStab->n_type)
        {
            case N_UNDF:
                nOffset += nNextOffset;
                nNextOffset = pStab->n_value;
                break;
            case N_SO:
                if((strLen = PICE_strlen(pName)))
                {
                    if(pName[strLen-1]!='/')
                    {
						ulCurrentFileNumber++;
                        if(PICE_strlen(szCurrentPath))
                        {
                            PICE_strcat(szCurrentPath,pName);
                            DPRINT((0,"FindTypeDefinition(): changing source file %s\n",szCurrentPath));
                        }
                        else
                        {
                            DPRINT((0,"FindTypeDefinition(): changing source file %s\n",pName));
                        }
                    }
                    else
                        PICE_strcpy(szCurrentPath,pName);
                }
                else
				{
                    szCurrentPath[0]=0;
				}
				break;
			case N_LSYM:
				// stab has no value -> must be type definition
				if(pStab->n_value == 0 && ulCurrentFileNumber==ulFileNumber)
				{
                    DPRINT((0,"FindTypeDefinition(): pre type definition %s\n",pName));
					// handle multi-line symbols
					if(strrchr(pName,'\\'))
					{
						if(PICE_strlen(szAccumulatedName))
						{
							PICE_strcat(szAccumulatedName,pName);
                            DPRINT((0,"FindTypeDefinition(): [1] accum. %s\n",szAccumulatedName));
						}
						else
						{
							PICE_strcpy(szAccumulatedName,pName);
                            DPRINT((0,"FindTypeDefinition(): [2] accum. %s\n",szAccumulatedName));
						}
                        szAccumulatedName[PICE_strlen(szAccumulatedName)-1]=0;
					}
					else
					{
                        DPRINT((0,"FindTypeDefinition(): [3] accum. %s\n",szAccumulatedName));
						if(PICE_strlen(szAccumulatedName)==0)
                        {
                            PICE_strcpy(szAccumulatedName,pName);
                        }
                        else
                        {
                            PICE_strcat(szAccumulatedName,pName);
                        }
                        pTypeString = szAccumulatedName;

                        pTypeSymbol = PICE_strchr(pTypeString,':');
						if(pTypeSymbol && (*(pTypeSymbol+1)=='t' || *(pTypeSymbol+1)=='T'))
						{
                            DPRINT((0,"FindTypeDefinition(): LSYM is type %s\n",pName));
							// parse it
							ulCurrentTypeNumber = ExtractTypeNumber(pTypeString);
							if(ulCurrentTypeNumber == ulTypeNumber)
							{
                                DPRINT((0,"FindTypeDefinition(): type definition %s\n",pTypeString));
								return pTypeString;
							}
						}
                        *szAccumulatedName=0;
					}
				}
				break;
        }
        pStab++;
    }

    return FindTypeDefinitionForCombinedTypes(pSymbols,ulTypeNumber,ulFileNumber);

}

//************************************************************************* 
// TruncateString() 
// 
//************************************************************************* 
LPSTR TruncateString(LPSTR p,char c)
{
	static char temp[1024];
	LPSTR pTemp;

	pTemp = temp;

	while(*p!=0 && *p!=c)
		*pTemp++ = *p++;

	*pTemp = 0;

	return temp;
}

//************************************************************************* 
// FindLocalsByAddress() 
// 
// find all locals for a given address by first looking up the function
// and then it's locals
//************************************************************************* 
PLOCAL_VARIABLE FindLocalsByAddress(ULONG addr)
{
    ULONG i;
    PSTAB_ENTRY pStab;
    LPSTR pStr,pName;
    int nStabLen;
    int nOffset=0,nNextOffset=0;
    PICE_SYMBOLFILE_HEADER* pSymbols;
    static char szCurrentFunction[256];
    static char szCurrentPath[256];
    LPSTR pFunctionName;
    ULONG start,end,strLen;
	ULONG ulTypeNumber,ulCurrentFileNumber=0;
	LPSTR pTypedef;
	ULONG ulNumLocalVars=0;

    DPRINT((0,"FindLocalsByAddress()\n"));

    pFunctionName = FindFunctionByAddress(addr,&start,&end);
    DPRINT((0,"FindLocalsByAddress(): pFunctionName = %s\n",pFunctionName));
    if(pFunctionName)
    {
        pSymbols = FindModuleSymbols(addr);
        if(pSymbols)
        {
            pStab = (PSTAB_ENTRY )((ULONG)pSymbols + pSymbols->ulOffsetToStabs);
            nStabLen = pSymbols->ulSizeOfStabs;
            pStr = (LPSTR)((ULONG)pSymbols + pSymbols->ulOffsetToStabsStrings);

            for(i=0;i<(nStabLen/sizeof(STAB_ENTRY));i++)
            {
                pName = &pStr[pStab->n_strx + nOffset];

                DPRINT((0,"FindLocalsByAddress(): %x %x %x %x %x\n",
                        pStab->n_strx,
                        pStab->n_type,
                        pStab->n_other,
                        pStab->n_desc,
                        pStab->n_value));

                switch(pStab->n_type)
                {
                    case N_UNDF:
                        nOffset += nNextOffset;
                        nNextOffset = pStab->n_value;
                        break;
                    case N_SO:
                        if((strLen = PICE_strlen(pName)))
                        {
                            if(pName[strLen-1]!='/')
                            {
                                ulCurrentFileNumber++;
                                if(PICE_strlen(szCurrentPath))
                                {
                                    PICE_strcat(szCurrentPath,pName);
                                    DPRINT((0,"changing source file %s\n",szCurrentPath));
                                }
                                else
                                {
                                    DPRINT((0,"changing source file %s\n",pName));
                                }
                            }
                            else
                                PICE_strcpy(szCurrentPath,pName);
                        }
                        else
						{
                            szCurrentPath[0]=0;
						}
                        break;
                    case N_LSYM:
						// if we're in the function we're looking for
                        if(szCurrentFunction[0] && PICE_strcmp(szCurrentFunction,pFunctionName)==0)
                        {
                            DPRINT((0,"local variable %.8X %.8X %.8X %.8X %.8X %s\n",pStab->n_strx,pStab->n_type,pStab->n_other,pStab->n_desc,pStab->n_value,pName));
							ulTypeNumber = ExtractTypeNumber(pName);
							DPRINT((0,"type number = %x\n",ulTypeNumber));
							if((pTypedef = FindTypeDefinition(pSymbols,ulTypeNumber,ulCurrentFileNumber)))
							{
								PICE_strcpy(local_vars[ulNumLocalVars].type_name,TruncateString(pTypedef,':'));
								PICE_strcpy(local_vars[ulNumLocalVars].name,TruncateString(pName,':'));
								local_vars[ulNumLocalVars].value = 0;
								local_vars[ulNumLocalVars].offset = pStab->n_value;
								local_vars[ulNumLocalVars].line = pStab->n_desc;
                                local_vars[ulNumLocalVars].bRegister = FALSE;
								ulNumLocalVars++;
							}
                        }
                        break;
					case N_PSYM:
						// if we're in the function we're looking for
                        if(szCurrentFunction[0] && PICE_strcmp(szCurrentFunction,pFunctionName)==0)
                        {
                            DPRINT((0,"parameter variable %.8X %.8X %.8X %.8X %.8X %s\n",pStab->n_strx,pStab->n_type,pStab->n_other,pStab->n_desc,pStab->n_value,pName));
							ulTypeNumber = ExtractTypeNumber(pName);
							DPRINT((0,"type number = %x\n",ulTypeNumber));
							if((pTypedef = FindTypeDefinition(pSymbols,ulTypeNumber,ulCurrentFileNumber)))
							{
								PICE_strcpy(local_vars[ulNumLocalVars].type_name,TruncateString(pTypedef,':'));
								PICE_strcpy(local_vars[ulNumLocalVars].name,TruncateString(pName,':'));
								local_vars[ulNumLocalVars].value = 0;
								local_vars[ulNumLocalVars].offset = pStab->n_value;
								ulNumLocalVars++;
							}
                        }
                        break;
                    case N_RSYM:
						// if we're in the function we're looking for
                        if(szCurrentFunction[0] && PICE_strcmp(szCurrentFunction,pFunctionName)==0)
                        {
                            DPRINT((0,"local variable %.8X %.8X %.8X %.8X %.8X %s\n",pStab->n_strx,pStab->n_type,pStab->n_other,pStab->n_desc,pStab->n_value,pName));
							ulTypeNumber = ExtractTypeNumber(pName);
							DPRINT((0,"type number = %x\n",ulTypeNumber));
							if((pTypedef = FindTypeDefinition(pSymbols,ulTypeNumber,ulCurrentFileNumber)))
							{
								PICE_strcpy(local_vars[ulNumLocalVars].type_name,TruncateString(pTypedef,':'));
								PICE_strcpy(local_vars[ulNumLocalVars].name,TruncateString(pName,':'));
								local_vars[ulNumLocalVars].value = 0;
								local_vars[ulNumLocalVars].offset = pStab->n_value;
								local_vars[ulNumLocalVars].line = pStab->n_desc;
                                local_vars[ulNumLocalVars].bRegister = TRUE;
								ulNumLocalVars++;
							}
                        }
                        break;
                    case N_FUN:
                        if(PICE_strlen(pName))
                        {
                            ULONG len;

	                        len=StrLenUpToWhiteChar(pName,":");
	                        PICE_strncpy(szCurrentFunction,pName,len);
                            szCurrentFunction[len]=0;
                            DPRINT((0,"function %s\n",szCurrentFunction));
                        }
						else
						{
                            DPRINT((0,"END of function %s\n",szCurrentFunction));
                            szCurrentFunction[0]=0;
							if(ulNumLocalVars)
							{
								*local_vars[ulNumLocalVars].name = 0;
								return local_vars;
							}
						}
                        break;
                }
                pStab++;
            }
        }
    }
	return NULL;
}

//************************************************************************* 
// FindSourceLineForAddress() 
// 
//************************************************************************* 
LPSTR FindSourceLineForAddress(ULONG addr,PULONG pulLineNumber,LPSTR* ppSrcStart,LPSTR* ppSrcEnd,LPSTR* ppFilename)
{
    ULONG i; // index for walking through STABS
    PSTAB_ENTRY pStab; // pointer to STABS
    LPSTR pStr,pName; // pointer to STAB strings and current STAB string
    int nStabLen;     // length of STAB section in bytes
    int nOffset=0,nNextOffset=0;    // offset and next offset in string table
    PICE_SYMBOLFILE_HEADER* pSymbols;   // pointer to module's STAB symbol table
    static char szCurrentFunction[256]; 
    static char szCurrentPath[256];
    static char szWantedPath[256];
    LPSTR pFunctionName; // name of function that brackets the current address
    ULONG start,end,strLen,ulMinValue=0xFFFFFFFF;
	LPSTR pSrcLine=NULL;
    BOOLEAN bFirstOccurence = TRUE;

    // lookup the functions name and start-end (external symbols)
    pFunctionName = FindFunctionByAddress(addr,&start,&end);
    if(pFunctionName)
    {
        // lookup the modules symbol table (STABS)
        pSymbols = FindModuleSymbols(addr);
        if(pSymbols)
        {
            // no source files so we don't need to lookup anything
            if(!pSymbols->ulNumberOfSrcFiles)
                return NULL;

            // prepare STABS access
            pStab = (PSTAB_ENTRY )((ULONG)pSymbols + pSymbols->ulOffsetToStabs);
            nStabLen = pSymbols->ulSizeOfStabs;
            pStr = (LPSTR)((ULONG)pSymbols + pSymbols->ulOffsetToStabsStrings);

            // walk over all STABS
            for(i=0;i<(nStabLen/sizeof(STAB_ENTRY));i++)
            {
                // the name string corresponding to the STAB
                pName = &pStr[pStab->n_strx + nOffset];

                // switch STAB type
                switch(pStab->n_type)
                {
                    // change offset of name strings
                    case N_UNDF:
                        nOffset += nNextOffset;
                        nNextOffset = pStab->n_value;
                        break;
                    // source file change
                    case N_SO:
                        DPRINT((0,"changing source file %s\n",pName));
                        // if filename has a length record it
                        if((strLen = PICE_strlen(pName)))
                        {
                            PICE_strcpy(szCurrentPath,pName);
                        }
                        // else empty filename
                        else
						{
                            szCurrentPath[0]=0;
						}
                        break;
                    // sub-source file change
                    case N_SOL:
                        DPRINT((0,"changing sub source file %s\n",pName));
                        // if filename has a length record it
                        if((strLen = PICE_strlen(pName)))
                        {
                            PICE_strcpy(szCurrentPath,pName);
                        }
                        // else empty filename
                        else
						{
                            szCurrentPath[0]=0;
						}
                        break;
                    // line number
                    case N_SLINE:
						// if we're in the function we're looking for
                        if(szCurrentFunction[0] && PICE_strcmp(szCurrentFunction,pFunctionName)==0)
                        {
                            //DPRINT((0,"code source line number #%u for addr. %x (function @ %x) ulMinValue = %x ulDelta = %x\n",pStab->n_desc,start+pStab->n_value,start,ulMinValue,(addr-(start+pStab->n_value))));

                            if(bFirstOccurence)
                            {
                                PICE_strcpy(szWantedPath,szCurrentPath);
                                DPRINT((0,"source file must be %s\n",szWantedPath));
                                bFirstOccurence = FALSE;
                            }

                            // we might have a match if our address is greater than the one in the STAB
                            // and we're lower or equal than minimum value
                            if(addr>=start+pStab->n_value &&
                               (addr-(start+pStab->n_value))<=ulMinValue &&
                               PICE_strcmp(szWantedPath,szCurrentPath)==0 )
                            {
                                ULONG j;
                                PICE_SYMBOLFILE_SOURCE* pSrc = (PICE_SYMBOLFILE_SOURCE*)((ULONG)pSymbols+pSymbols->ulOffsetToSrcFiles);

	                            DPRINT((0,"code source line number #%u for addr. %x found!\n",pStab->n_desc,start+pStab->n_value));

                                // compute new minimum
								ulMinValue = addr-(start+pStab->n_value);

                                // if we have a pointer for storage of line number, store it
                                if(pulLineNumber)
                                    *pulLineNumber = pStab->n_desc;

                                // NB: should put this somewhere else so that it's not done all the time
                                // if we have source files at all
                                DPRINT((0,"%u source files @ %x\n",pSymbols->ulNumberOfSrcFiles,pSrc));

                                // for all source files in this module
                                for(j=0;j<pSymbols->ulNumberOfSrcFiles;j++)
                                {
                                    LPSTR pSlash;

                                    // make basename
                                    pSlash = strrchr(pSrc->filename,'/');
                                    if(!pSlash)
                                        pSlash = pSrc->filename;
                                    else
                                        pSlash++;

                                    // if base name matches current path we have found the correct source file
                                    if(PICE_strcmpi(pSlash,szCurrentPath)==0)
                                    {
                                        // the linenumber
                                        ULONG k = pStab->n_desc;

                                        DPRINT((0,"found src file %s @ %x\n",pSrc->filename,pSrc));

                                        // store the pointer to the filename
                                        if(ppFilename)
                                            *ppFilename = pSrc->filename;

                                        if(pSrc->ulOffsetToNext > sizeof(PICE_SYMBOLFILE_SOURCE))
                                        {
                                            // get a pointer to the source file (right after the file header)
                                            pSrcLine = (LPSTR)((ULONG)pSrc+sizeof(PICE_SYMBOLFILE_SOURCE));

                                            // store the source start and end address
                                            if(ppSrcStart)
                                                *ppSrcStart = pSrcLine;
                                            if(ppSrcEnd)
                                                *ppSrcEnd = pSrcLine+pSrc->ulOffsetToNext-sizeof(PICE_SYMBOLFILE_SOURCE);

                                            // goto to the right line
                                            while(--k)
                                            {
                                                while(*pSrcLine!=0 && *pSrcLine!=0x0a && *pSrcLine!=0x0d)
                                                    pSrcLine++;
                                                if(!IsAddressValid((ULONG)pSrcLine))
                                                    return NULL;
                                                pSrcLine++;
                                            }

										    if(ulMinValue == 0)
											    return pSrcLine;
                                        }
                                        else
                                        {
                                            DPRINT((0,"src file descriptor found, but contains no source\n"));
                                        }

										break;
                                    }
                                    (ULONG)pSrc += pSrc->ulOffsetToNext;
                                }
                            }
                        }
                        break;
                    // a function symbol
                    case N_FUN:
                        // if it has a length it's the start of a function
                        if(PICE_strlen(pName))
                        {
                            ULONG len;

                            // extract the name only, the type string is of no use here
	                        len=StrLenUpToWhiteChar(pName,":");
	                        PICE_strncpy(szCurrentFunction,pName,len);
                            szCurrentFunction[len]=0;

                            DPRINT((0,"function %s\n",szCurrentFunction));
                        }
                        // else it's the end of a function
						else
						{
                            DPRINT((0,"END of function %s\n",szCurrentFunction));

							szCurrentFunction[0]=0;

                            // in case we haven't had a zero delta match we return from here
							if(pSrcLine)
								return pSrcLine;
						}
                        break;
                }
                pStab++;
            }
        }
    }
    return NULL;
}

//************************************************************************* 
// FindAddressForSourceLine() 
// 
//************************************************************************* 
BOOLEAN FindAddressForSourceLine(ULONG ulLineNumber,LPSTR pFilename,struct module* pMod,PULONG pValue)
{
    ULONG i;
    PSTAB_ENTRY pStab;
    LPSTR pStr,pName;
    int nStabLen;
    int nOffset=0,nNextOffset=0;
    PICE_SYMBOLFILE_HEADER* pSymbols;
    static char szCurrentFunction[256];
    static char szCurrentPath[256];
    ULONG strLen,addr,ulMinValue=0xFFFFFFFF;
    BOOLEAN bFound = FALSE;

    DPRINT((0,"FindAddressForSourceLine(%u,%s,%x)\n",ulLineNumber,pFilename,(ULONG)pMod));

    if(pMod == &fake_kernel_module)
        addr = KERNEL_START;
    else
        addr = (ULONG)pMod;

    addr += sizeof(struct module);

    pSymbols = FindModuleSymbols(addr);
    if(pSymbols)
    {
        pStab = (PSTAB_ENTRY )((ULONG)pSymbols + pSymbols->ulOffsetToStabs);
        nStabLen = pSymbols->ulSizeOfStabs;
        pStr = (LPSTR)((ULONG)pSymbols + pSymbols->ulOffsetToStabsStrings);

        for(i=0;i<(nStabLen/sizeof(STAB_ENTRY));i++)
        {
            pName = &pStr[pStab->n_strx + nOffset];

            switch(pStab->n_type)
            {
                case N_UNDF:
                    nOffset += nNextOffset;
                    nNextOffset = pStab->n_value;
                    break;
                case N_SO:
                    if((strLen = PICE_strlen(pName)))
                    {
                        if(pName[strLen-1]!='/')
                        {
                            if(PICE_strlen(szCurrentPath))
                            {
                                PICE_strcat(szCurrentPath,pName);
                                DPRINT((0,"changing source file %s\n",szCurrentPath));
                            }
                            else
                            {
                                DPRINT((0,"changing source file %s\n",pName));
                                PICE_strcpy(szCurrentPath,pName);
                            }
                        }
                        else
                            PICE_strcpy(szCurrentPath,pName);
                    }
                    else
					{
                        szCurrentPath[0]=0;
					}
                    break;
                case N_SLINE:
					// if we're in the function we're looking for
                    if(PICE_strcmpi(pFilename,szCurrentPath)==0)
                    {
                        if(pStab->n_desc>=ulLineNumber && (pStab->n_desc-ulLineNumber)<=ulMinValue)
                        {
                            ulMinValue = pStab->n_desc-ulLineNumber;

                            DPRINT((0,"code source line number #%u for offset %x in function @ %s)\n",pStab->n_desc,pStab->n_value,szCurrentFunction));
                            addr = FindFunctionInModuleByName(szCurrentFunction,pMod);
                            if(addr)
                            {
                                *pValue = addr + pStab->n_value;
                                bFound = TRUE;
                            }
                        }
                    }
                    break;
                case N_FUN:
                    if(PICE_strlen(pName))
                    {
                        ULONG len;

	                    len=StrLenUpToWhiteChar(pName,":");
	                    PICE_strncpy(szCurrentFunction,pName,len);
                        szCurrentFunction[len]=0;
                        DPRINT((0,"function %s\n",szCurrentFunction));
                    }
					else
					{
                        DPRINT((0,"END of function %s\n",szCurrentFunction));
						szCurrentFunction[0]=0;
					}
                    break;
            }
            pStab++;
        }
    }
    return bFound;
}

//************************************************************************* 
// ListSymbolStartingAt() 
// 
//************************************************************************* 
ULONG ListSymbolStartingAt(struct module* pMod,PICE_SYMBOLFILE_HEADER* pSymbols,ULONG index,LPSTR pOutput)
{
    ULONG i;
	Elf32_Sym* pElfSym;
	LPSTR pElfStr;
	Elf32_Shdr* pElfShdr;

    DPRINT((0,"ListSymbolStartingAt(%x,%u)\n",(ULONG)pSymbols,index));
    DPRINT((0,"ListSymbolStartingAt(): ulOffsetToGlobals = %x ulSizeofGlobals = %x\n",pSymbols->ulOffsetToGlobals,pSymbols->ulSizeOfGlobals));

	pElfSym = (Elf32_Sym*)((ULONG)pSymbols+pSymbols->ulOffsetToGlobals);
	pElfStr = (LPSTR)((ULONG)pSymbols+pSymbols->ulOffsetToGlobalsStrings);
    pElfShdr = (Elf32_Shdr*)((ULONG)pSymbols+pSymbols->ulOffsetToHeaders);

    pElfSym +=index;

	for(i=index;i<(pSymbols->ulSizeOfGlobals/sizeof(Elf32_Sym));i++)
	{
        LPSTR pName = &pElfStr[pElfSym->st_name];

        DPRINT((0,"ListSymbolStartingAt(): [%u] %.8X %.8X %s\n",i,pElfSym->st_info,pElfSym->st_shndx,pName));

		if((ELF32_ST_BIND(pElfSym->st_info)==STB_GLOBAL || ELF32_ST_BIND(pElfSym->st_info)==STB_LOCAL || ELF32_ST_BIND(pElfSym->st_info)==STB_WEAK) &&
			(ELF32_ST_TYPE(pElfSym->st_info)==STT_FUNC || ELF32_ST_TYPE(pElfSym->st_info)==STT_OBJECT)&& 
		   (pElfSym->st_shndx<SHN_LORESERVE || pElfSym->st_shndx==SHN_ABS || pElfSym->st_shndx==SHN_COMMON) )
		{
            ULONG section_flags = pElfShdr[pElfSym->st_shndx].sh_flags;
            ULONG start;

            if(pElfSym->st_shndx!=SHN_ABS && pElfSym->st_shndx!=SHN_COMMON)
            {
                if(pMod != &fake_kernel_module)
                {
			        start = ((ULONG)pMod+pElfShdr[pElfSym->st_shndx].sh_offset);

			        start = (start+pElfShdr[pElfSym->st_shndx].sh_addralign)&~(pElfShdr[pElfSym->st_shndx].sh_addralign-1);

			        start += pElfSym->st_value;
                }
                else
                {
			        start = pElfSym->st_value;
                }
            }
            else
            {
                start = FindFunctionInModuleByNameViaKsyms(pMod,pName);
            }

            PICE_sprintf(pOutput,"%.8X (%s) %s\n",start,(section_flags&SHF_EXECINSTR)?"TEXT":"DATA",pName);

            if((i+1)<(pSymbols->ulSizeOfGlobals/sizeof(Elf32_Sym)) )
                return i+1;

        }
        pElfSym++;
    }

    return 0;
}

//************************************************************************* 
// SanityCheckExports() 
// 
//************************************************************************* 
BOOLEAN SanityCheckExports(void)
{
    BOOLEAN bResult = FALSE;
    ULONG i,ulValue,incr;

    Print(OUTPUT_WINDOW,"pICE: sanity-checking exports...\n");

    if(fake_kernel_module.nsyms && fake_kernel_module.syms)
    {
        incr = (fake_kernel_module.nsyms/4);
        if(!incr)incr = 1;
        for(i=0;i<fake_kernel_module.nsyms;i+=incr)
        {
            if(ScanExports((char*)fake_kernel_module.syms[i].name,&ulValue) )
            {
                if(!(i%25))
                {
                    ClrLine(wWindow[OUTPUT_WINDOW].y + wWindow[OUTPUT_WINDOW].usCurY);
                    PICE_sprintf(tempSym,"pICE: sanity-checking exports %u/%u",
                                          i,
                                          fake_kernel_module.nsyms);
                    PutChar(tempSym,1,wWindow[OUTPUT_WINDOW].y + wWindow[OUTPUT_WINDOW].usCurY);
                }

                if(fake_kernel_module.syms[i].value != ulValue)
                {
                    PICE_sprintf(tempSym,"pICE: %s doesn't match (%.8X != %.8X)\n",
                                 fake_kernel_module.syms[i].name,
                                 fake_kernel_module.syms[i].value,
                                 ulValue);
                    Print(OUTPUT_WINDOW,tempSym);

                    return FALSE;
                }
            }
        }

        bResult = TRUE;
    }

    return bResult;
}

//************************************************************************* 
// LoadExports() 
// 
//************************************************************************* 
BOOLEAN LoadExports(void)
{
	struct file* pf;
    BOOLEAN bResult = TRUE;

	ENTER_FUNC();
	
    Print(OUTPUT_WINDOW,"pICE: loading exports...\n");
	pf = filp_open("/boot/System.map",O_RDONLY,S_IRUSR);

	if(IS_ERR(pf))
    {
        Print(OUTPUT_WINDOW,"pICE: no System.map in /boot\n");
	    pf = filp_open("/System.map",O_RDONLY,S_IRUSR);
    }

    if(!IS_ERR(pf))
	{
		mm_segment_t oldfs;
		size_t len;

        if(pf->f_op)
        {
		    DPRINT((0,"pf = %x pf->f_op->read @ %x\n",(ULONG)pf,(ULONG)pf->f_op->read));
		    DPRINT((0,"pf->f_count = %x p->f_flags %x\n",pf->f_count,pf->f_flags));

		    len = pf->f_dentry->d_inode->i_size;
            if(len)
            {
		        DPRINT((0,"file len = %d\n",len));

		        pExports = vmalloc(len+1);

		        DPRINT((0,"pExports = %x\n",pExports));

                PICE_sprintf(tempSym,"pICE: exports loaded @ %x (size %x)\n",pExports,len);
		        Print(OUTPUT_WINDOW,tempSym);
		        
                if(pExports)
		        {
        		    oldfs = get_fs(); set_fs(KERNEL_DS);
                    ulExportLen = len;
			        ((PUCHAR)pExports)[len]=0;
			        if(len == pf->f_op->read(pf,pExports,len,&pf->f_pos))
			        {
				        DPRINT((0,"success reading system map!\n"));
			        }
    		        set_fs(oldfs);
		        }

            }
        }

		filp_close(pf,0);
	}
    else
    {
        Print(OUTPUT_WINDOW,"pICE: no System.map in /\n");
        Print(OUTPUT_WINDOW,"pICE: could not load exports...\n");
        bResult = FALSE;
    }

    LEAVE_FUNC();

    return bResult;
}

//************************************************************************* 
// UnloadExports() 
// 
//************************************************************************* 
void UnloadExports(void)
{
	ENTER_FUNC();
	if(pExports)
	{	
		DPRINT((0,"freeing %x\n",pExports));
		vfree(pExports);
        pExports = NULL;
	}
    LEAVE_FUNC();
}

//************************************************************************* 
// LoadSymbols() 
// 
//************************************************************************* 
PICE_SYMBOLFILE_HEADER* LoadSymbols(LPSTR filename)
{
	struct file* pf;
    PICE_SYMBOLFILE_HEADER* pSymbols=NULL;

	ENTER_FUNC();
	
    if(ulNumSymbolsLoaded<DIM(apSymbols))
    {
	    pf = filp_open(filename,O_RDONLY,S_IRUSR);
	    if(!IS_ERR(pf))
	    {
		    mm_segment_t oldfs;
		    size_t len;

            DPRINT((0,"pf = %x\n",pf));
            if(pf->f_op)
            {
		        DPRINT((0,"pf = %x pf->f_op->read @ %x\n",(ULONG)pf,(ULONG)pf->f_op->read));
		        DPRINT((0,"pf->f_count = %x p->f_flags %x\n",pf->f_count,pf->f_flags));

		        len = pf->f_dentry->d_inode->i_size;
		        DPRINT((0,"file len = %d\n",len));

                if(len)
                {
		            pSymbols = vmalloc(len+1);
		            DPRINT((0,"pSymbols = %x\n",pSymbols));

		            if(pSymbols)
		            {
        		        oldfs = get_fs(); set_fs(KERNEL_DS);
			            if(len == pf->f_op->read(pf,(PVOID)pSymbols,len,&pf->f_pos))
			            {
				            DPRINT((0,"LoadSymbols(): success reading symbols!\n"));
				            DPRINT((0,"LoadSymbols(): pSymbols->magic = %X\n",pSymbols->magic));
			            }
        		        set_fs(oldfs);


						if(pSymbols->magic == PICE_MAGIC)
						{
                            DPRINT((0,"magic = %X\n",pSymbols->magic));
	                        DPRINT((0,"name = %s\n",pSymbols->name));;
                            DPRINT((0,"ulOffsetToHeaders,ulSizeOfHeader = %X,%X\n",pSymbols->ulOffsetToHeaders,pSymbols->ulSizeOfHeader));
                            DPRINT((0,"ulOffsetToGlobals,ulSizeOfGlobals = %X,%X\n",pSymbols->ulOffsetToGlobals,pSymbols->ulSizeOfGlobals));
                            DPRINT((0,"ulOffsetToGlobalsStrings,ulSizeOfGlobalsStrings = %X,%X\n",pSymbols->ulOffsetToGlobalsStrings,pSymbols->ulSizeOfGlobalsStrings));
                            DPRINT((0,"ulOffsetToStabs,ulSizeOfStabs = %X,%X\n",pSymbols->ulOffsetToStabs,pSymbols->ulSizeOfStabs));
                            DPRINT((0,"ulOffsetToStabsStrings,ulSizeOfStabsStrings = %X,%X\n",pSymbols->ulOffsetToStabsStrings,pSymbols->ulSizeOfStabsStrings));
                            DPRINT((0,"ulOffsetToSrcFiles,ulNumberOfSrcFiles = %X,%X\n",pSymbols->ulOffsetToSrcFiles,pSymbols->ulNumberOfSrcFiles));
							DPRINT((0,"pICE: symbols loaded for module \"%s\" @ %x\n",pSymbols->name,pSymbols));
							apSymbols[ulNumSymbolsLoaded++]=pSymbols;
						}
						else
						{
    						DPRINT((0,"LoadSymbols(): freeing %x\n",pSymbols));
							DPRINT((0,"pICE: symbols file \"%s\" corrupt\n",filename));
	    					vfree(pSymbols);
						}
		            }

                }
            }

		    filp_close(pf,0);
	    }
        else
        {
			DPRINT((0,"pICE: could not load symbols for %s...\n",filename));
        }
    }

	LEAVE_FUNC();

    return pSymbols;
}

//************************************************************************* 
// ReloadSymbols() 
// 
//************************************************************************* 
BOOLEAN ReloadSymbols(void)
{
    BOOLEAN bResult;

	ENTER_FUNC();

    UnloadSymbols();
    
    bResult = LoadSymbolsFromConfig(TRUE);

    LEAVE_FUNC();

    return bResult;
}

//************************************************************************* 
// UnloadSymbols() 
// 
//************************************************************************* 
void UnloadSymbols()
{
    ULONG i;

	ENTER_FUNC();

    if(ulNumSymbolsLoaded)
	{	
        for(i=0;i<ulNumSymbolsLoaded;i++)
        {
    		DPRINT((0,"freeing [%u] %x\n",i,apSymbols[i]));
	    	vfree(apSymbols[i]);
            apSymbols[i] = NULL;
        }
        ulNumSymbolsLoaded = 0;
	}
    LEAVE_FUNC();
}

//************************************************************************* 
// LoadSymbolsFromConfig() 
// 
//************************************************************************* 
BOOLEAN LoadSymbolsFromConfig(BOOLEAN bIgnoreBootParams)
{
	struct file* pf;
	LPSTR pConfig,pConfigEnd,pTemp;
	char temp[256];
	ULONG line = 1;
    BOOLEAN bResult = FALSE;

	ENTER_FUNC();

	pf = filp_open("/etc/pice.conf",O_RDONLY,0600);
	if(!IS_ERR(pf))
	{
		mm_segment_t oldfs;
		size_t len;

        DPRINT((0,"pf = %x\n",pf));
        if(pf->f_op)
        {
		    DPRINT((0,"pf = %x pf->f_op->read @ %x\n",(ULONG)pf,(ULONG)pf->f_op->read));
		    DPRINT((0,"pf->f_count = %x p->f_flags %x\n",pf->f_count,pf->f_flags));

		    len = pf->f_dentry->d_inode->i_size;
		    DPRINT((0,"file len = %d\n",len));

            if(len)
            {
		        pConfig = vmalloc(len+1);
		        DPRINT((0,"pConfig = %x\n",pConfig));
        		oldfs = get_fs(); set_fs(KERNEL_DS);
			    if(len == pf->f_op->read(pf,(PVOID)pConfig,len,&pf->f_pos))
				{
	    		    set_fs(oldfs);

					pConfigEnd = pConfig + len;

					while(pConfig<pConfigEnd)
					{
						// skip leading spaces
						while(*pConfig==' ' && pConfig<pConfigEnd)
							pConfig++;
						// get ptr to temporary
						pTemp = temp;
						// fill in temporary with symbol path
						while(*pConfig!=0 && *pConfig!=0x0a && *pConfig!=0x0d && pConfig<pConfigEnd)
							*pTemp++ = *pConfig++;
						// finish up symbol path string
						*pTemp = 0;
						// skip any line ends
						while((*pConfig==0x0a || *pConfig==0x0d) && pConfig<pConfigEnd)
							pConfig++;

						// finally try to load the symbols
						if(PICE_strlen(temp))
						{
                            PICE_SYMBOLFILE_HEADER *pSymbols;

                            // boot parameter
                            if(*temp == '!')
                            {
                                if(!bIgnoreBootParams)
                                {
                                    if(!PICE_strlen(szBootParams))
                                    {
                                        PICE_strcpy(szBootParams,temp+1);
                                        DPRINT((0,"pICE: boot params = %s\n",szBootParams));
                                    }
                                    else
                                    {
                                        DPRINT((0,"pICE: boot params already exist! ignoring...\n",szBootParams));
                                    }
                                }
                            }
                            // options
                            else if(*temp == '+')
                            {
                                if(PICE_strlen(temp)>1)
                                {
                                    if(PICE_strcmp(temp,"+vga")==0)
                                    {
                                        eTerminalMode = TERMINAL_MODE_VGA_TEXT;
                                        DPRINT((0,"pICE: eTerminalMode = TERMINAL_MODE_VGA_TEXT\n"));
                                    }
                                    else if(PICE_strcmp(temp,"+hercules")==0)
                                    {
                                        eTerminalMode = TERMINAL_MODE_HERCULES_GRAPHICS;
                                        DPRINT((0,"pICE: eTerminalMode = TERMINAL_MODE_HERCULES_GRAPHICS\n"));
                                    }
                                    else if(PICE_strcmp(temp,"+serial")==0)
                                    {
                                        eTerminalMode = TERMINAL_MODE_SERIAL;
                                        DPRINT((0,"pICE: eTerminalMode = TERMINAL_MODE_SERIAL\n"));
                                    }
                                }
                                else
                                {
                                    DPRINT((0,"pICE: found option, but no value\n"));
                                }
                            }
                            // comment
                            else if(*temp == '#')
                            {
                                DPRINT((0,"comment out\n"));
                            }
                            // symbol file name/path
                            else
                            {
							    pSymbols = LoadSymbols(temp);
                                if(pSymbols)
                                {
                                    PICE_SYMBOLFILE_SOURCE* pSrc;
                                    LPSTR p;

                                    pSrc = (PICE_SYMBOLFILE_SOURCE*)((ULONG)pSymbols + pSymbols->ulOffsetToSrcFiles);
                                    pCurrentSymbols = pSymbols;
                                    p = strrchr(pSrc->filename,'/');
                                    if(p)
                                    {
                                        PICE_strcpy(szCurrentFile,p+1);
                                    }
                                    else
                                    {
                                        PICE_strcpy(szCurrentFile,pSrc->filename);
                                    }
                                }
                            }
						}
						else
						{
                            DPRINT((0,"invalid line [%u] in config!\n",line));
						}
						line++;
					}
				}
				else
				{
	    		    set_fs(oldfs);
				}
			}
		}
	    filp_close(pf,0);
        bResult = TRUE;
	}
	else
	{
		DPRINT((0,"pICE: config file not found! No symbols loaded.\n"));
		DPRINT((0,"pICE: Please make sure to create a file /etc/pice.conf\n"));
		DPRINT((0,"pICE: if you want to have symbols for any module loaded.\n"));
	}

    LEAVE_FUNC();

    return bResult;
}


//************************************************************************* 
// EVALUATION OF EXPRESSIONS
//************************************************************************* 

//************************************************************************* 
// SkipSpaces() 
// 
//************************************************************************* 
void SkipSpaces(void)
{
	while(pExpression[ulIndex]==' ')
		ulIndex++;
};

//************************************************************************* 
// FindGlobalStabSymbol() 
// 
//************************************************************************* 
BOOLEAN FindGlobalStabSymbol(LPSTR pExpression,PULONG pValue,PULONG pulTypeNumber,PULONG pulFileNumber)
{
    ULONG i;
    PSTAB_ENTRY pStab;
    LPSTR pStr,pName;
    int nStabLen;
    int nOffset=0,nNextOffset=0,nLen,strLen;
    PICE_SYMBOLFILE_HEADER* pSymbols;
	ULONG ulTypeNumber;
	static char SymbolName[1024];
    static char szCurrentPath[256];
	ULONG ulCurrentFileNumber=0;
    LPSTR pTypeDefIncluded;
    ULONG addr;

    // must have a current module
	if(pCurrentMod)
	{
        // in case we query for the kernel we need to use the fake kernel module
        if(pCurrentMod == &fake_kernel_module)
        {
            addr = KERNEL_START;
        }
        else
        {
            addr = (ULONG)pCurrentMod;
        }

        addr += sizeof(struct module);

        // find the symbols for the module
		pSymbols = FindModuleSymbols(addr);
		if(pSymbols)
		{
            // prepare table access
			pStab = (PSTAB_ENTRY )((ULONG)pSymbols + pSymbols->ulOffsetToStabs);
			nStabLen = pSymbols->ulSizeOfStabs;
			pStr = (LPSTR)((ULONG)pSymbols + pSymbols->ulOffsetToStabsStrings);
            // starting at file 0
			*pulFileNumber = 0;

            // go through stabs
			for(i=0;i<(nStabLen/sizeof(STAB_ENTRY));i++)
			{
				pName = &pStr[pStab->n_strx + nOffset];

				switch(pStab->n_type)
				{
                    // an N_UNDF symbol marks a change of string table offset
					case N_UNDF:
						nOffset += nNextOffset;
						nNextOffset = pStab->n_value;
						break;
                    // a source file symbol
					case N_SO:
						if((strLen = PICE_strlen(pName)))
						{
							if(pName[strLen-1]!='/')
							{
								ulCurrentFileNumber++;
								if(PICE_strlen(szCurrentPath))
								{
									PICE_strcat(szCurrentPath,pName);
									DPRINT((0,"changing source file %s\n",szCurrentPath));
								}
								else
								{
									DPRINT((0,"changing source file %s\n",pName));
								}
							}
							else
								PICE_strcpy(szCurrentPath,pName);
						}
						else
						{
							szCurrentPath[0]=0;
						}
						break;
					case N_GSYM:
                        // symbol-name:type-identifier type-number =
						nLen = StrLenUpToWhiteChar(pName,":");
						PICE_strncpy(SymbolName,pName,nLen);
						SymbolName[nLen] = 0;
						if(PICE_strcmpi(SymbolName,pExpression)==0)
						{
                            DPRINT((0,"global symbol %s\n",pName));
                            // extract type-number from stab
							ulTypeNumber = ExtractTypeNumber(pName);
							DPRINT((0,"type number = %x\n",ulTypeNumber));
							*pulTypeNumber = ulTypeNumber;
                            // look for symbols address in external symbols
							*pValue = FindFunctionInModuleByName(SymbolName,pCurrentMod);
							DPRINT((0,"value = %x\n",*pValue));
							*pulFileNumber = ulCurrentFileNumber;
							DPRINT((0,"file = %x\n",ulCurrentFileNumber));
                            if((pTypeDefIncluded = PICE_strchr(pName,'=')) )
                            {
                                DPRINT((0,"symbol includes type definition (%s)\n",pTypeDefIncluded));
                            }
    						return TRUE;
						}
						break;
				}
				pStab++;
			}
		}
	}
	return FALSE;
}

//************************************************************************* 
// ExtractToken() 
// 
//************************************************************************* 
void ExtractToken(LPSTR pStringToken)
{
	while(isalpha(pExpression[ulIndex]) || isdigit(pExpression[ulIndex]) || pExpression[ulIndex]=='_')
	{
		*pStringToken++=pExpression[ulIndex++];
		*pStringToken=0;
	}
}

//************************************************************************* 
// ExtractTypeName() 
// 
//************************************************************************* 
LPSTR ExtractTypeName(LPSTR p)
{
    static char temp[1024];
    ULONG i;

    DPRINT((0,"ExtractTypeName(%s)\n",p));

    for(i=0;IsAddressValid((ULONG)p) && *p!=0 && *p!=':';i++,p++)
        temp[i] = *p;

    if(!IsAddressValid((ULONG)p) )
    {
        DPRINT((0,"hit invalid page %x!\n",(ULONG)p));
    }

    temp[i]=0;

    return temp;
}

//************************************************************************* 
// ExtractNumber() 
// 
//************************************************************************* 
LONG ExtractNumber(LPSTR p)
{
    LONG lMinus = 1,lBase;
    ULONG lNumber = 0;

    DPRINT((0,"ExtractNumber(): %s\n",p));

    if(!IsAddressValid((ULONG)p) )
    {
        DPRINT((0,"ExtractNumber(): [1] invalid page %x hit!\n",p));
        return 0;
    }

    if(*p == '-')
    {
        lMinus = -1;
        p++;
    }

    if(!IsAddressValid((ULONG)p) )
    {
        DPRINT((0,"ExtractNumber(): [2] invalid page %x hit!\n",p));
        return 0;
    }

    if(*p != '0') // non-octal -> decimal number
        lBase = 10;
    else
        lBase = 8;
    
    if(!IsAddressValid((ULONG)p) )
    {
        DPRINT((0,"ExtractNumber(): [3] invalid page %x hit!\n",p));
        return 0;
    }

    while(isdigit(*p))
    {
        lNumber *= lBase;
        lNumber += *p-'0';
        p++;
        if(!IsAddressValid((ULONG)p) )
        {
            DPRINT((0,"ExtractNumber(): [4] invalid page %x hit!\n",p));
            return 0;
        }
    }

    return (lNumber*lMinus);
}

//************************************************************************* 
// ExtractArray() 
// 
//************************************************************************* 
BOOLEAN ExtractArray(PVRET pvr,LPSTR p)
{
    ULONG index_typenumber,type_number;
    ULONG lower_bound,upper_bound;
    LPSTR pTypeDef;

    DPRINT((0,"ExtractArray(%s)\n",p));

    // index-type index-type-number;lower;upper;element-type-number
    pvr->bArrayType = TRUE;
    p++;
    index_typenumber = ExtractTypeNumber(p);
    p = PICE_strchr(p,';');
    if(p)
    {
        p++;
        lower_bound = ExtractNumber(p);
        p = PICE_strchr(p,';');
        if(p)
        {
            p++;

            upper_bound = ExtractNumber(p);
            p = PICE_strchr(p,';');
            if(p)
            {
                p++;

                type_number = ExtractTypeNumber(p);

                DPRINT((0,"ExtractArray(): %x %x %x %x\n",index_typenumber,lower_bound,upper_bound,type_number));

                pTypeDef = FindTypeDefinition(pvr->pSymbols,type_number,pvr->file);
                if(pTypeDef)
                {
                    PICE_strcpy(pvr->type_name,ExtractTypeName(pTypeDef));
                    pvr->type = type_number;
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

//************************************************************************* 
// ExtractStructMembers() 
// 
//************************************************************************* 
PVRET ExtractStructMembers(PVRET pvr,LPSTR p)
{
    ULONG len;
    static char member_name[128];
    LONG bit_offset,bit_size,type_number,byte_size;
    static VRET vr;
    LPSTR pTypeDef,pEqual;

    DPRINT((0,"ExtractStructMembers(): %s\n",p));

    PICE_memset(&vr,0,sizeof(vr));

    // name:type-number,bit-offset,bit-size
	len=StrLenUpToWhiteChar(p,":");
    if(len)
    {
        // extract member name
	    PICE_strncpy(member_name,p,len);
        member_name[len]=0;
        DPRINT((0,"ExtractStructMembers(): member_name = %s\n",member_name));

        // go to char following ':'
        p += (len+1);
        if(IsAddressValid((ULONG)p) )
        {
            type_number = ExtractTypeNumber(p);
            DPRINT((0,"ExtractStructMembers(): type_number = %x\n",type_number));

            vr.type = type_number;
            
            pEqual = PICE_strchr(p,')');
            // see if it includes type def
            if(pEqual)
            {
                p = pEqual+1;
                if(*p == '=')
                {
                    p++;
                    if(*p == 'a')
                    {
                        DPRINT((0,"ExtractStructMembers(): member is array\n"));
                        vr.bArrayType = TRUE;
                        p = PICE_strchr(p,';');
                        p = PICE_strchr(p,';');
                        p = PICE_strchr(p,';');
                        if(p)
                            p++;

                        type_number = ExtractTypeNumber(p);
                        vr.father_type = type_number;
                    }
                    else if(*p == '*')
                    {
                        DPRINT((0,"ExtractStructMembers(): member is ptr\n"));
                        vr.bPtrType = TRUE;
                        type_number = ExtractTypeNumber(p);
                        DPRINT((0,"ExtractStructMembers(): type_number = %x\n",type_number));
                        vr.father_type = type_number;
                    }
                    else if(*p == 'u')
                    {
                        DPRINT((0,"ExtractStructMembers(): member is union\n"));
                        while(*p!=';' && *(p+1)!=';' && *p!=0)p++;
                    }
                }
            }

            p = PICE_strchr(p,',');
            if(p)
            {
                p++;
                bit_offset = ExtractNumber(p);
                DPRINT((0,"ExtractStructMembers(): bit_offset = %x\n",bit_offset));
                p = PICE_strchr(p,',');
                if(p)
                {
                    p++;

                    bit_size = ExtractNumber(p);
                    DPRINT((0,"ExtractStructMembers(): bit_size = %x\n",bit_size));

                    vr.address = pvr->value + bit_offset/8;
                    vr.file = pvr->file;
                    vr.size = bit_size;
                    PICE_strcpy(vr.name,member_name);
                    byte_size = (bit_size+1)/8;
                    if(!byte_size)
                        byte_size = 4;
                    pvr->address = pvr->value;
                    if(IsRangeValid(vr.address,byte_size))
                    {
                        switch(byte_size)
                        {
                            case 1:
                                vr.value = *(PUCHAR)vr.address;
                                break;
                            case 2:
                                vr.value = *(PUSHORT)vr.address;
                                break;
                            case 4:
                                vr.value = *(PULONG)vr.address;
                                break;
                        }
                    }

                    DPRINT((0,"ExtractStructMembers(): member %s type %x bit_offset %x bit_size%x\n",member_name,type_number,bit_offset,bit_size));

                    pTypeDef = FindTypeDefinition(pvr->pSymbols,type_number,pvr->file);
                    if(pTypeDef)
                    {
                        DPRINT((0,"ExtractStructMembers(): pTypedef= %s\n",pTypeDef));
                        PICE_strcpy(vr.type_name,ExtractTypeName(pTypeDef));
                        pTypeDef = PICE_strchr(pTypeDef,':');
                        if(pTypeDef)
                        {
                            pTypeDef++;
                            type_number = ExtractTypeNumber(pTypeDef);
                            DPRINT((0,"ExtractStructMembers(): type_number = %x\n",type_number));
                            vr.father_type = type_number;
                        }
                    }
                }
            }
        }
    }

    return &vr;
}

//************************************************************************* 
// EvaluateSymbol() 
// 
//************************************************************************* 
BOOLEAN EvaluateSymbol(PVRET pvr,LPSTR pToken)
{
    LPSTR pTypeDef,pTypeName,pTypeBase,pSemiColon,pStructMembers;
    BOOLEAN bDone = FALSE;
    ULONG ulType,ulBits,ulBytes;
    LONG lLowerRange,lUpperRange,lDelta;
    static char type_def[2048];

    DPRINT((0,"EvaluateSymbol(%s)\n",pToken));

	if(FindGlobalStabSymbol(pToken,&pvr->value,&pvr->type,&pvr->file))
    {
        DPRINT((0,"EvaluateSymbol(%s) pvr->value = %x pvr->type = %x\n",pToken,pvr->value,pvr->type));
        while(!bDone)
        {
            if(!(pTypeDef = FindTypeDefinition(pvr->pSymbols,pvr->type,pvr->file)))
                break;

            PICE_strcpy(type_def,pTypeDef);

            pTypeDef = type_def;

            pTypeName = ExtractTypeName(pTypeDef);

            DPRINT((0,"%s %s\n",pTypeName,pToken));

            PICE_strcpy(pvr->type_name,pTypeName);

            pTypeBase = PICE_strchr(pTypeDef,'=');

            if(!pTypeBase)
                return FALSE;

            pTypeBase++;

            switch(*pTypeBase)
            {
                case '(': // type reference
                    ulType = ExtractTypeNumber(pTypeBase);
                    DPRINT((0,"%x is a type reference to %x\n",pvr->type,ulType));
                    pvr->type = ulType;
                    break;
                case 'r': // subrange
                    pTypeBase++;
                    ulType = ExtractTypeNumber(pTypeBase);
                    DPRINT((0,"%x is sub range of %x\n",pvr->type,ulType));
                    if(pvr->type == ulType)
                    {
                        DPRINT((0,"%x is a self reference\n",pvr->type));
                        pSemiColon = PICE_strchr(pTypeBase,';');
                        pSemiColon++;
                        lLowerRange = ExtractNumber(pSemiColon);
                        pSemiColon = PICE_strchr(pSemiColon,';');
                        pSemiColon++;
                        lUpperRange = ExtractNumber(pSemiColon);
                        lDelta = lUpperRange-lLowerRange;
                        DPRINT((0,"bounds %x-%x range %x\n",lLowerRange,lUpperRange,lDelta));
                        ulBits=0;
                        do
                        {
                            ulBits++;
                            lDelta /= 2;
                        }while(lDelta);
                        ulBytes = (ulBits+1)/8;
                        if(!ulBytes)
                            ulBytes = 4;
                        DPRINT((0,"# of bytes = %x\n",ulBytes));
                        pvr->address = pvr->value;
                        if(IsRangeValid(pvr->value,ulBytes))
                        {
                            switch(ulBytes)
                            {
                                case 1:
                                    pvr->value = *(PUCHAR)pvr->value;
                                    break;
                                case 2:
                                    pvr->value = *(PUSHORT)pvr->value;
                                    break;
                                case 4:
                                    pvr->value = *(PULONG)pvr->value;
                                    break;
                            }
                        }
                        bDone=TRUE;
                    }
                    else
                        pvr->type = ulType;
                    break;
                case 'a': // array type
                    DPRINT((0,"%x array\n",pvr->type));
                    pTypeBase++;
                    if(!ExtractArray(pvr,pTypeBase))
                    {
                        bDone = TRUE;
                        pvr->error = 1;
                    }
                    break;
                case '*': // ptr type
                    DPRINT((0,"%x is ptr to\n",pvr->type));
                    bDone = TRUE; // meanwhile
                    break;
                case 's': // struct type [name:T(#,#)=s#membername1:(#,#),#,#;membername1:(#,#),#,#;;]
                    // go past 's'
                    pTypeBase++;

                    // extract the the struct size
                    lLowerRange = ExtractNumber(pTypeBase);
                    DPRINT((0,"%x struct size = %x\n",pvr->type,lLowerRange));

                    // skip over the digits
                    while(isdigit(*pTypeBase))
                        pTypeBase++;

                    // the structs address is is value
                    pvr->address = pvr->value;
                    pvr->bStructType = TRUE;

                    // decode the struct members. pStructMembers now points to first member name
                    pStructMembers = pTypeBase;

                    while(pStructMembers && *pStructMembers && *pStructMembers!=';' && ulNumStructMembers<DIM(vrStructMembers))
                    {
                        DPRINT((0,"EvaluateSymbol(): member #%u\n",ulNumStructMembers));
                        // put this into our array
                        vrStructMembers[ulNumStructMembers] = *ExtractStructMembers(pvr,pStructMembers);

                        if(!PICE_strlen(vrStructMembers[ulNumStructMembers].type_name))
                        {
                            ULONG i;
                            PVRET pvrThis = &vrStructMembers[ulNumStructMembers];

                            DPRINT((0,"EvaluateSymbol(): no type name\n"));
                            for(i=0;i<ulNumStructMembers;i++)
                            {
                                DPRINT((0,"EvaluateSymbol(): vr[i].type_name = %s\n",vrStructMembers[i].type_name));
                                DPRINT((0,"EvaluateSymbol(): vr[i].name = %s\n",vrStructMembers[i].name));
                                DPRINT((0,"EvaluateSymbol(): vr[i].address = %.8X\n",vrStructMembers[i].address));
                                DPRINT((0,"EvaluateSymbol(): vr[i].value = %.8X\n",vrStructMembers[i].value));
                                DPRINT((0,"EvaluateSymbol(): vr[i].size = %.8X\n",vrStructMembers[i].size));
                                DPRINT((0,"EvaluateSymbol(): vr[i].type = %.8X\n",vrStructMembers[i].type));
                                if(pvrThis->type == vrStructMembers[i].type)
                                {
                                    PICE_strcpy(pvrThis->type_name,vrStructMembers[i].type_name);
                                    pvrThis->bArrayType = vrStructMembers[i].bArrayType;
                                    pvrThis->bPtrType = vrStructMembers[i].bPtrType;
                                    pvrThis->bStructType = vrStructMembers[i].bStructType;
                                    break;
                                }
                            }
                        }

                        DPRINT((0,"EvaluateSymbol(): vr.type_name = %s\n",vrStructMembers[ulNumStructMembers].type_name));
                        DPRINT((0,"EvaluateSymbol(): vr.name = %s\n",vrStructMembers[ulNumStructMembers].name));
                        DPRINT((0,"EvaluateSymbol(): vr.address = %.8X\n",vrStructMembers[ulNumStructMembers].address));
                        DPRINT((0,"EvaluateSymbol(): vr.value = %.8X\n",vrStructMembers[ulNumStructMembers].value));
                        DPRINT((0,"EvaluateSymbol(): vr.size = %.8X\n",vrStructMembers[ulNumStructMembers].size));
                        DPRINT((0,"EvaluateSymbol(): vr.type = %.8X\n",vrStructMembers[ulNumStructMembers].type));

                        ulNumStructMembers++;

                        // skip to next ':'
                        pStructMembers = PICE_strchr(pStructMembers,';');
                        pStructMembers = PICE_strchr(pStructMembers,':');
                        if(pStructMembers)
                        {
                            DPRINT((0,"EvaluateSymbol(): ptr is now %s\n",pStructMembers));
                            // go back to where member name starts
                            while(*pStructMembers!=';') 
                                pStructMembers--;
                            // if ';' present, go to next char
                            if(pStructMembers)
                                pStructMembers++;
                        }
                    }

                    bDone = TRUE; // meanwhile
                    break;
                case 'u': // union type
                    DPRINT((0,"%x union\n",pvr->type));
                    bDone = TRUE; // meanwhile
                    break;
                case 'e': // enum type
                    DPRINT((0,"%x enum\n",pvr->type));
                    bDone = TRUE; // meanwhile
                    break;
                default:
                    DPRINT((0,"DEFAULT %x\n",pvr->type));
                    bDone = TRUE;
                    break;
            }

        }
        return TRUE;
	}
    return FALSE;
}

//************************************************************************* 
// Symbol() 
// 
// Symbol := v
//************************************************************************* 
BOOLEAN Symbol(PVRET pvr)
{
	char SymbolToken[128];

	ExtractToken(SymbolToken);

	DPRINT((0,"SymbolToken = %s\n",SymbolToken));

    return EvaluateSymbol(pvr,SymbolToken);
}

//************************************************************************* 
// Expression() 
// 
// Expression := Symbol | Symbol->Symbol
//************************************************************************* 
BOOLEAN Expression(PVRET pvr)
{
	if(!Symbol(pvr))
        return FALSE;

	return TRUE;
}

//************************************************************************* 
// Evaluate() 
// 
//************************************************************************* 
void Evaluate(PICE_SYMBOLFILE_HEADER* pSymbols,LPSTR p)
{
    ULONG i;

	PICE_memset(&vr,0,sizeof(vr));
	vr.pSymbols = pSymbols;

	pExpression = p;
	ulIndex=0;
    ulNumStructMembers=0;
	if(Expression(&vr))
	{
		DPRINT((0,"\nOK!\n"));
		DPRINT((0,"value = %x type = %x\n",vr.value,vr.type));
        if(vr.bStructType)
        {
            PICE_sprintf(tempSym,"struct %s %s @ %x\n",vr.type_name,p,vr.address);
            Print(OUTPUT_WINDOW,tempSym);
            for(i=0;i<ulNumStructMembers;i++)
            {
                if(vrStructMembers[i].bArrayType)
                {
                    PICE_sprintf(tempSym,"[%.8X %.8X] %s %s[%u]\n",
                            vrStructMembers[i].address,
                            vrStructMembers[i].size/8,
                            vrStructMembers[i].type_name,
                            vrStructMembers[i].name,
                            vrStructMembers[i].size/8);
                }
                else if(vrStructMembers[i].bPtrType)
                {
                    PICE_sprintf(tempSym,"[%.8X %.8X] %s* %s -> %x (%u)\n",
                            vrStructMembers[i].address,
                            vrStructMembers[i].size/8,
                            vrStructMembers[i].type_name,
                            vrStructMembers[i].name,
                            vrStructMembers[i].value,
                            vrStructMembers[i].value);
                }
                else
                {
                    PICE_sprintf(tempSym,"[%.8X %.8X] %s %s = %x (%u)\n",
                            vrStructMembers[i].address,
                            vrStructMembers[i].size/8,
                            vrStructMembers[i].type_name,
                            vrStructMembers[i].name,
                            vrStructMembers[i].value,
                            vrStructMembers[i].value);
                }
                Print(OUTPUT_WINDOW,tempSym);
            }
        }
        else if(vr.bArrayType)
        {
            Print(OUTPUT_WINDOW,"array\n");
        }
        else
        {
            PICE_sprintf(tempSym,"%s %s @ %x = %x (%u)\n",vr.type_name,p,vr.address,vr.value,vr.value);
            Print(OUTPUT_WINDOW,tempSym);
        }
	}
	else
	{
		DPRINT((0,"\nERROR: code %x\n",vr.error));
	}
}
