/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    util.c

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
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>

#include <defines.h>


////////////////////////////////////////////////////
// GLOBALS
////
// output string
char tempUtil[1024];
char tempFlowChanges[256];

struct mm_struct* my_init_mm=(struct mm_struct*)NULL;
struct module **pmodule_list=NULL;

ULONG TwoPagesForPhysMem[2*PAGE_SIZE];

// scancode to ASCII conversion
typedef struct tagSCANTOASCII
{
	UCHAR s,a;
}SCANTOASCII;

// scancode to ASCII table
SCANTOASCII ucScanToAscii_GER[]=
{
// german keyboard
	{16,'q'},{17,'w'},{18,'e'},{19,'r'},{20,'t'},
	{21,'z'},{22,'u'},{23,'i'},{24,'o'},{25,'p'},
	{30,'a'},{31,'s'},{32,'d'},{33,'f'},{34,'g'},
	{35,'h'},{36,'j'},{37,'k'},{38,'l'},
	{44,'y'},{45,'x'},{46,'c'},{47,'v'},{48,'b'},
	{49,'n'},{50,'m'},
	{2,'1'},{3,'2'},{4,'3'},{ 5,'4'},{ 6,'5'},
	{7,'6'},{8,'7'},{9,'8'},{10,'9'},{11,'0'},
	{12,'ß'}, // 239 = &szlig;
	{0x39,' '},{0x35,'-'},{0x34,'.'},{0x1b,'+'}
};

SCANTOASCII ucScanToAscii_US[]=
{
// US keyboard
    {16,'q'},{17,'w'},{18,'e'},{19,'r'},
	{20,'t'},{21,'y'},{22,'u'},{23,'i'},
	{24,'o'},{25,'p'},{30,'a'},{31,'s'},
	{32,'d'},{33,'f'},{34,'g'},{35,'h'},
	{36,'j'},{37,'k'},{38,'l'},{44,'z'},
	{45,'x'},{46,'c'},{47,'v'},{48,'b'},
	{49,'n'},{50,'m'},{2,'1'},{3,'2'},
	{4,'3'},{5,'4'},{6,'5'},{7,'6'},
	{8,'7'},{9,'8'},{10,'9'},{11,'0'},{12,'ß'},
	{0x39,' '},{0x35,'-'},{0x34,'.'},{0x1b,'+'}
};

SCANTOASCII ucShiftScanToAscii_GER[]=
{
// german keyboard SHIFTED
	{16,'Q'},{17,'W'},{18,'E'},{19,'R'},{20,'T'},
	{21,'Z'},{22,'U'},{23,'I'},{24,'O'},{25,'P'},
	{30,'A'},{31,'S'},{32,'D'},{33,'F'},{34,'G'},
	{35,'H'},{36,'J'},{37,'K'},{38,'L'},
	{44,'Y'},{45,'X'},{46,'C'},{47,'V'},{48,'B'},
	{49,'N'},{50,'M'},
	{2,'!'},{3,'\"'}, // " // (fixes mc syntax highlighting)
	                {4,'@'}, // is pragraph sign on keyboard
					        { 5,'$'},{ 6,'%'},
	{7,'&'},{8,'/'},{9,'('},{10,')'},{11,'='},
	{12,'?'},
	{0x39,' '},{0x35,'_'},{0x34,':'},{0x1b,'*'}
};

SCANTOASCII ucShiftScanToAscii_US[]=
{
// US keyboard SHIFTED
	{16,'Q'},{17,'W'},{18,'E'},{19,'R'},
	{20,'T'},{21,'Y'},{22,'U'},{23,'I'},
	{24,'O'},{25,'P'},{30,'A'},{31,'S'},
	{32,'D'},{33,'F'},{34,'G'},{35,'H'},
	{36,'J'},{37,'K'},{38,'L'},{44,'Z'},
	{45,'X'},{46,'C'},{47,'V'},{48,'B'},
	{49,'N'},{50,'M'},{2,'!'},{3,'\"'},
	{4,'@'},{5,'$'},{6,'%'},{7,'&'},
	{8,'/'},{9,'('},{10,')'},{11,'='},{12,'?'},
	{0x39,' '},{0x35,'?'},{0x34,':'},{0x1b,'*'}
};

////////////////////////////////////////////////////
// FUNCTIONS
////

//*************************************************************************
// PICE_memset()
//
//*************************************************************************
void PICE_memset(void* p,unsigned char c,int sz)
{
    unsigned char *p2 = (unsigned char *)p;
    while(sz--)
        *p2++ = c;
}

//*************************************************************************
// PICE_memcpy()
//
//*************************************************************************
void PICE_memcpy(void* t,void* s,int sz)
{
    memcpy(t,s,sz);
}

//*************************************************************************
// PICE_isprint()
//
//*************************************************************************
BOOLEAN PICE_isprint(char c)
{
	BOOLEAN bResult = FALSE;

	if((ULONG)c>=0x20 && (ULONG)c<=0x7f)
		bResult = TRUE;

	return bResult;
}

//*************************************************************************
// PICE_strchr()
//
//*************************************************************************
char* PICE_strchr(char* s,char c)
{
    while(IsAddressValid((ULONG)s) && *s)
    {
        if(*s == c)
            return s;
        s++;
    }
#ifdef DEBUG
    if(!IsAddressValid((ULONG)s) )
    {
        DPRINT((0,"PICE_strchr(): ********************\n"));
        DPRINT((0,"PICE_strchr(): EXCEPTION @ %.8X\n",(ULONG)s));
        DPRINT((0,"PICE_strchr(): ********************\n"));
    }
#endif

    return NULL;
}

//*************************************************************************
// PICE_strncpy()
//
//*************************************************************************
char* PICE_strncpy(char* s1,char* s2,int len)
{
	ULONG len2 =  PICE_strlen(s2);

	if(len<len2)
		PICE_memcpy(s1,s2,len2+1);
	else
		PICE_memcpy(s1,s2,len);

	return s1;
}

//*************************************************************************
// PICE_strcpy()
//
//*************************************************************************
char* PICE_strcpy(char* s1,char* s2)
{
	ULONG len2 =  PICE_strlen(s2);

	PICE_memcpy(s1,s2,len2+1);

	return s1;
}

//*************************************************************************
// PICE_strcat()
//
//*************************************************************************
char* PICE_strcat(char* s1,char* s2)
{
	ULONG len1 = PICE_strlen(s1);
	ULONG len2 = PICE_strlen(s2);

	PICE_memcpy(&s1[len1],s2,len2+1);

	return s1;
}

//*************************************************************************
// PICE_toupper()
//
//*************************************************************************
char PICE_toupper(char c)
{
	if(c>='a' && c<='z')
		c = (c-'a')+'A';

	return c;
}

//*************************************************************************
// PICE_strncmpi()
//
// my version of strncmpi()
//*************************************************************************
ULONG PICE_strncmpi(char* s1,char* s2,ULONG len)
{
ULONG result=1;

    while(len &&
		  IsAddressValid((ULONG)s1) && *s1 && // not end of string
          IsAddressValid((ULONG)s2) && *s2 && // not end of string
          PICE_toupper(*s1)==PICE_toupper(*s2) ) // char are the same except case
    {
        s1++;
        s2++;
		len--;
    }
	// strings same length
    if(len==0)
        result=0;

    return result;
}

//*************************************************************************
// PICE_strcmpi()
//
// my version of strcmp()
//*************************************************************************
ULONG PICE_strcmpi(char* s1,char* s2)
{
ULONG result=1;

    while(IsAddressValid((ULONG)s1) && *s1 && // not end of string
          IsAddressValid((ULONG)s2) && *s2 && // not end of string
          PICE_toupper(*s1)==PICE_toupper(*s2) ) // char are the same except case
    {
        s1++;
        s2++;
    }
	// strings same length
    if(*s1==0 && *s2==0)
        result=0;

    return result;
}

//*************************************************************************
// PICE_strrev()
//
// my version of strrev()
//*************************************************************************
char* PICE_strrev(char* s)
{
ULONG i,j,len=PICE_strlen(s)-1;
char c;

	for(i=0,j=len;i<j;i++,j--)
	{
		c=s[i]; s[i]=s[j]; s[j]=c;
	}

	return s;
}

//*************************************************************************
// PICE_strlen()
//
// my version of strlen()
//
// does a page validity check on every character in th string
//*************************************************************************
USHORT PICE_strlen(char* s)
{
	USHORT i;

	for(i=0;IsAddressValid((ULONG)&s[i]) && s[i]!=0 && i<PAGE_SIZE;i++);

    if(IsAddressValid((ULONG)&s[i]) && s[i]==0)
        return i;

    return 0;
}

#ifndef LINUX
//*************************************************************************
// GetShortName()
//
// separates module name from path
//*************************************************************************
LPSTR GetShortName(LPSTR p)
{
ULONG i;

	// scan backwards till backslash or start
	for(i=PICE_strlen(p);p[i]!='\\' && &p[i]!=p;i--);
	// it's not start, inc. counter
	if(&p[i]!=p)i++;

	// return section of string containing mod name
	return &p[i];
}

//*************************************************************************
// CopyWideToAnsi()
//
// copy wide string to ANSI string
//*************************************************************************
void CopyWideToAnsi(LPSTR pAnsi,PWSTR pWide)
{
ULONG j;

	for(j=0;pWide[j]!=0;j++)
	{
        if((char)(pWide[j]>>8)==0)
		    pAnsi[j]=(char)(pWide[j]);
        else
            pAnsi[j]=0x20;
	}
	pAnsi[j]=0;

}
#endif // LINUX

//*************************************************************************
// IsAddressValid()
//
//*************************************************************************
BOOLEAN IsAddressValid(ULONG address)
{
	pgd_t * pPGD;
	pmd_t * pPMD;
	pte_t * pPTE;
	BOOLEAN bResult = FALSE;
	struct mm_struct* p = NULL;
	struct task_struct* my_current = (struct task_struct*)0xFFFFE000;

	address &= (~(PAGE_SIZE-1));

    // if we're in DebuggerShell() we live on a different stack
    if(bInDebuggerShell)
	    (ULONG)my_current &= ulRealStackPtr;
    else
	    my_current = current;

    if(address < TASK_SIZE)
	{
		p = my_current->mm;
	}
    else
    {
        p = my_init_mm;
    }

	if(p)
	{
		pPGD = pgd_offset(p,address);
        if(pPGD && pgd_val(*pPGD)&_PAGE_PRESENT)
        {
            // not large page
            if(!(pgd_val(*pPGD)&_PAGE_4M))
			{
				pPMD = pmd_offset(pPGD,address);
				if(pPMD)
				{
					pPTE = pte_offset(pPMD,address);
					if(pPTE)
					{
						bResult = pte_present(*pPTE);
					}
				}
			}
			// large page
			else
			{
				bResult = TRUE;
			}
		}
	}

	return bResult;
}


//*************************************************************************
// IsAddressWriteable()
//
// returns:
//  TRUE    if adress/page is writeable
//  FALSE   if adress/page is not writeable
//
//*************************************************************************
BOOLEAN IsAddressWriteable(ULONG address)
{
	pgd_t * pPGD;
	pmd_t * pPMD;
	pte_t * pPTE;
	BOOLEAN bResult = FALSE;
	struct mm_struct* p = NULL;
	struct task_struct* my_current = (struct task_struct*)0xFFFFE000;

	address &= (~(PAGE_SIZE-1));

    // if we're in DebuggerShell() we live on a different stack
    if(bInDebuggerShell)
	    (ULONG)my_current &= ulRealStackPtr;
    else
	    my_current = current;

    if(address < TASK_SIZE)
	{
		p = my_current->mm;
	}
    else
    {
        p = my_init_mm;
    }

	if(p)
	{
		pPGD = pgd_offset(p,address);
        if(pPGD && pgd_val(*pPGD)&_PAGE_PRESENT)
        {
            // not large page
            if(!(pgd_val(*pPGD)&_PAGE_4M))
			{
		        bResult |= pgd_val(*pPGD) & _PAGE_RW;

				pPMD = pmd_offset(pPGD,address);
				if(pPMD)
				{
		    		bResult |= pmd_val(*pPMD) & _PAGE_RW;

					pPTE = pte_offset(pPMD,address);
					if(pPTE)
					{
						if( pte_present(*pPTE) )
						    bResult |= pte_write(*pPTE);
					}
				}
			}
			// large page
			else
			{
				bResult |= pgd_val(*pPGD) & _PAGE_RW;
			}
		}
	}

	return bResult;
}


//*************************************************************************
// SetAddressWriteable()
//
//*************************************************************************
BOOLEAN SetAddressWriteable(ULONG address,BOOLEAN bSet)
{
	pgd_t * pPGD;
	pmd_t * pPMD;
	pte_t * pPTE;
	BOOLEAN bResult = FALSE;
	struct mm_struct* p = NULL;
	struct task_struct* my_current = (struct task_struct*)0xFFFFE000;

	address &= (~(PAGE_SIZE-1));

    // if we're in DebuggerShell() we live on a different stack
    if(bInDebuggerShell)
	    (ULONG)my_current &= ulRealStackPtr;
    else
	    my_current = current;

    if(address < TASK_SIZE)
	{
		p = my_current->mm;
	}
    else
    {
        p = my_init_mm;
    }

	if(p)
	{
		pPGD = pgd_offset(p,address);
        if(pPGD && pgd_val(*pPGD)&_PAGE_PRESENT)
        {
            // not large page
            if(!(pgd_val(*pPGD)&_PAGE_4M))
			{
				pPMD = pmd_offset(pPGD,address);
				if(pPMD)
				{
					pPTE = pte_offset(pPMD,address);
					if(pPTE)
					{
						if( pte_present(*pPTE) )
						{
                            if( bSet )
								pte_mkwrite(*pPTE);
                            else
								pte_wrprotect(*pPTE);
                            bResult = TRUE;
                        }
					}
				}
			}
			// large page
			else
			{
                if( bSet )
                    pgd_val(*pPGD) |= _PAGE_RW;
                else
                    pgd_val(*pPGD) &= ~_PAGE_RW;
                bResult = TRUE;
			}
		}
	}

	return bResult;
}
//*************************************************************************
// IsRangeValid()
//
// scan range for page present
//*************************************************************************
BOOLEAN IsRangeValid(ULONG Addr,ULONG Length)
{
ULONG i,NumPages,PageNum;

	// need to only touch one byte per page
	// calculate PICE_number of pages to touch
	NumPages=(Length+(PAGE_SIZE-1))>>12;

	// calculate PICE_number of page
	PageNum=Addr>>PAGE_SHIFT;

	// touch all pages containing range
	for(i=0;i<NumPages;i++)
	{
		// if any one page is invalid range is invalid
		if(!IsAddressValid((ULONG)((PageNum+i)*PAGE_SIZE)) )
			return FALSE;
	}

	return TRUE;
}

//*************************************************************************
// IsModuleLoaded()
//
//*************************************************************************
struct module* IsModuleLoaded(LPSTR p)
{
    struct module* pMod;
    DPRINT((0,"IsModuleLoaded(%s)\n",p));

    if(pmodule_list)
    {
        pMod = *pmodule_list;
        do
        {
            DPRINT((0,"module (%x) %s\n",pMod->size,pMod->name));
            if(pMod->size && strcmp(p,(LPSTR)pMod->name) == 0)
            {
                DPRINT((0,"module %s is loaded!\n",pMod->name));
                return pMod;
            }
        }while((pMod = pMod->next));
    }

    return NULL;
}

//*************************************************************************
// GetGDTPtr()
//
// return flat address of GDT
//*************************************************************************
PGDT GetGDTPtr(void)
{
ULONG gdtr[2];
PGDT pGdt;

    ENTER_FUNC();

	__asm__("sgdt %0;":"=m" (gdtr));
	pGdt=(PGDT)(((ULONG)(gdtr[1]<<16))|((ULONG)(gdtr[0]>>16)));

    LEAVE_FUNC();

	return pGdt;
}

//*************************************************************************
// GetLinearAddress()
//
// return flat address for SEGMENT:OFFSET
//*************************************************************************
ULONG GetLinearAddress(USHORT Segment,ULONG Offset)
{
    PGDT pGdt;
    ULONG result=0;
    PDESCRIPTOR pSel;
    USHORT OriginalSegment=Segment;

    ENTER_FUNC();

	pSel=(struct tagDESCRIPTOR*)&Segment;

	// get GDT pointer
	pGdt=GetGDTPtr();
    DPRINT((0,"GetLinearAddress(): pGDT = %.8X\n",pGdt));
    DPRINT((0,"GetLinearAddress(): original Segment:Offset = %.4X:%.8X\n",Segment,Offset));

	// see if segment selector is in LDT
	if(pSel->Ti)
	{
		DPRINT((0,"GetLinearAddress(): Segment is in LDT\n"));
		// get LDT selector
		__asm__("
			sldt %%ax
			mov %%ax,%0"
			:"=m" (Segment));
		if(Segment)
		{
			DPRINT((0,"GetLinearAddress(): no LDT\n"));
			// get LDT selector
			pGdt=(PGDT)((pGdt[pSel->Val].Base_31_24<<24)|
					   (pGdt[pSel->Val].Base_23_16<<16)|
				       (pGdt[pSel->Val].Base_15_0));
			if(!IsRangeValid((ULONG)pGdt,0x8) )
				pGdt=0;
		}
		else
		{
			pGdt=0;
		}
	}

	if(pGdt && Segment)
	{
        DPRINT((0,"GetLinearAddress(): Segment:Offset = %.4X:%.8X\n",Segment,Offset));
		result=pGdt[OriginalSegment>>3].Base_15_0|
			   (pGdt[OriginalSegment>>3].Base_23_16<<16)|
			   (pGdt[OriginalSegment>>3].Base_31_24<<24);
		result+=Offset;
	}
	DPRINT((0,"GetLinearAddress(%.4X:%.8X)=%.8X\n",OriginalSegment,Offset,result));

    LEAVE_FUNC();

	return result;
}

//*************************************************************************
// ShowRunningMsg()
//
// place RUNNING message
//*************************************************************************
void ShowRunningMsg(void)
{
    ENTER_FUNC();

    SetForegroundColor(COLOR_TEXT);
	SetBackgroundColor(COLOR_CAPTION);
	ClrLine(wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy);
	PutChar(" Linux is running... (Press CTRL-F to stop) ",1,wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy);
    ResetColor();

    LEAVE_FUNC();
}

//*************************************************************************
// ShowStoppedMsg()
//
// place STOPPED message
//*************************************************************************
void ShowStoppedMsg(void)
{
    ENTER_FUNC();

    SetForegroundColor(COLOR_TEXT);
	SetBackgroundColor(COLOR_CAPTION);
	ClrLine(wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy);
	PutChar(" Stopped... (Type 'x' to continue) ",1,wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy);
    ResetColor();

    LEAVE_FUNC();
}

//*************************************************************************
// SetHardwareBreakPoint()
//
//*************************************************************************
void SetHardwareBreakPoint(ULONG ulAddress,ULONG ulReg)
{
    ULONG mask = 0x300;
    ULONG enable_mask = 0x3;

    DPRINT((0,"SetHardwareBreakPoint(%x,DR%x)\n",ulAddress,ulReg));

    enable_mask <<= (ulReg*2);
    mask |= enable_mask;

    DPRINT((0,"mask = %x\n",mask));

	__asm__ __volatile__
	("
		xorl %%eax,%%eax
		mov %%eax,%%dr6
        mov %%dr7,%%eax
        orl %0,%%eax
		mov %%eax,%%dr7
	"
	:
	:"m" (mask)
	:"eax");

    switch(ulReg)
    {
        case 0:
            __asm__ __volatile__
            ("
        		mov %0,%%eax
		        mov %%eax,%%dr0
             "
             :
             :"m" (ulAddress)
             :"eax");
             break;
        case 1:
            __asm__ __volatile__
            ("
        		mov %0,%%eax
		        mov %%eax,%%dr1
             "
             :
             :"m" (ulAddress)
             :"eax");
             break;
        case 2:
            __asm__ __volatile__
            ("
        		mov %0,%%eax
		        mov %%eax,%%dr2
             "
             :
             :"m" (ulAddress)
             :"eax");
             break;
        case 3:
            __asm__ __volatile__
            ("
        		mov %0,%%eax
		        mov %%eax,%%dr3
             "
             :
             :"m" (ulAddress)
             :"eax");
             break;
    }
}

//*************************************************************************
// SetHardwareBreakPoints()
//
// install HW breakpoints
//*************************************************************************
void SetHardwareBreakPoints(void)
{
ULONG i;
ULONG mask;
ULONG LinAddr0,LinAddr1,LinAddr2,LinAddr3;
PULONG LinAddr[4]={&LinAddr0,&LinAddr1,&LinAddr2,&LinAddr3};

    ENTER_FUNC();

	// cancel all debug activity
	__asm__("
		pushl %eax
		xorl %eax,%eax
		mov %eax,%dr6
		mov %eax,%dr7
		popl %eax");
	// build DR7 mask
	for(mask=0,i=0;i<4;i++)
	{
		mask<<=2;
		if(Bp[i].Active && Bp[i].Used && !Bp[i].Virtual)
		{
			mask|=0x03;
			*LinAddr[3-i]=Bp[i].LinearAddress;
			DPRINT((0,"breakpoint %u at %.8X\n",i,Bp[i].LinearAddress));
		}
	}
	if(mask)
	{
		__asm__("
			pushl %%eax
			movl %0,%%eax
			andl $0x000000FF,%%eax
			orl $0x300,%%eax
			mov %%eax,%%dr7
			mov %1,%%eax
			mov %%eax,%%dr0
			mov %2,%%eax
			mov %%eax,%%dr1
			mov %3,%%eax
			mov %%eax,%%dr2
			mov %4,%%eax
			mov %%eax,%%dr3
			popl %%eax"
			:
			:"m" (mask),"m" (LinAddr0),"m" (LinAddr1),"m" (LinAddr2),"m" (LinAddr3));
	}

    LEAVE_FUNC();
}

//*************************************************************************
// IsCallInstrAtEIP()
//
// check if instruction at CS:EIP changes program flow
//*************************************************************************
BOOLEAN IsCallInstrAtEIP(void)
{
PUCHAR linear;
BOOLEAN result=FALSE;

    ENTER_FUNC();
	DPRINT((0,"IsCallInstrAtEIP()\n"));

	linear=(PUCHAR)GetLinearAddress(CurrentCS,CurrentEIP);
	if(IsRangeValid((ULONG)linear,2))
	{
		if(*linear== 0xE8 || // call
		   (*linear== 0xFF && ( ((*(linear+1)>>3)&0x7)==0x2 || ((*(linear+1)>>3)&0x7)==0x3) ) || // call
		   *linear== 0x9A || // call
		   *linear== 0xF2 || // REP
		   *linear== 0xF3)   // REP
			result=TRUE;
	}

    LEAVE_FUNC();

	return result;
}


//*************************************************************************
// IsRetAtEIP()
//
// check if instruction at CS:EIP is a return instruction
//*************************************************************************
BOOLEAN IsRetAtEIP(void)
{
	PUCHAR linear;
	BOOLEAN bResult = FALSE;

    ENTER_FUNC();
	DPRINT((0,"IsRetAtEIP()\n"));

	linear=(PUCHAR)GetLinearAddress(CurrentCS,CurrentEIP);

    switch(*linear)
    {
        case 0xc2:
        case 0xc3:
        case 0xca:
        case 0xcb:
        case 0xcf: // IRET/IRETD
			bResult = TRUE;
            break;
    }

    LEAVE_FUNC();

    return bResult;
}

//*************************************************************************
// VisualizeFlags()
//
// display CPU EFLAGS as string
//*************************************************************************
LPSTR VisualizeFlags(ULONG EFlags)
{
    static UCHAR FlagNames[]={'c',0,'p',0,'a',0,'z','s','t','i','d','o'};
    ULONG i,j;
    static char temp[32];

	for(j=0,i=0;i<sizeof(FlagNames);i++)
	{
		if(FlagNames[i]!=0)
		{
			if(EFlags&1)
				temp[j++] = PICE_toupper(FlagNames[i]);
			else
				temp[j++] = FlagNames[i];
			temp[j++]=' ';
		}
		EFlags>>=1;
	}
	temp[j]=0;
	PICE_strrev(temp);
	return temp;
}

//*************************************************************************
// DisplayRegs()
//
// display CPU registers
//*************************************************************************
void DisplayRegs(void)
{
    char tempDisplayRegs[48];

    ENTER_FUNC();

//	Clear(REGISTER_WINDOW);
	Home(REGISTER_WINDOW);
	// EAX
    Print(REGISTER_WINDOW,"EAX=");
	PICE_sprintf(tempDisplayRegs,"%.8X",CurrentEAX);
	if(OldEAX!=CurrentEAX)
	{
		SetForegroundColor(WHITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldEAX!=CurrentEAX)
	{
        ResetColor();
	}

	// EBX
    Print(REGISTER_WINDOW," EBX=");
	PICE_sprintf(tempDisplayRegs,"%.8X",CurrentEBX);
	if(OldEBX!=CurrentEBX)
	{
		SetForegroundColor(WHITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldEBX!=CurrentEBX)
	{
        ResetColor();
	}

	// ECX
    Print(REGISTER_WINDOW," ECX=");
	PICE_sprintf(tempDisplayRegs,"%.8X",CurrentECX);
	if(OldECX!=CurrentECX)
	{
		SetForegroundColor(WHITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldECX!=CurrentECX)
	{
        ResetColor();
	}

	// EDX
    Print(REGISTER_WINDOW," EDX=");
	PICE_sprintf(tempDisplayRegs,"%.8X",CurrentEDX);
	if(OldEDX!=CurrentEDX)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldEDX!=CurrentEDX)
	{
		ResetColor();
	}

	// ESI
    Print(REGISTER_WINDOW," ESI=");
	PICE_sprintf(tempDisplayRegs,"%.8X",CurrentESI);
	if(OldESI!=CurrentESI)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldESI!=CurrentESI)
	{
		ResetColor();
	}

	// EDI
    Print(REGISTER_WINDOW," EDI=");
	PICE_sprintf(tempDisplayRegs,"%.8X\n",CurrentEDI);
	if(OldEDI!=CurrentEDI)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldEDI!=CurrentEDI)
	{
		ResetColor();
	}

	// EBP
    Print(REGISTER_WINDOW,"EBP=");
	PICE_sprintf(tempDisplayRegs,"%.8X",CurrentEBP);
	if(OldEBP!=CurrentEBP)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldEBP!=CurrentEBP)
	{
		ResetColor();
	}

	// ESP
    Print(REGISTER_WINDOW," ESP=");
	PICE_sprintf(tempDisplayRegs,"%.8X",CurrentESP);
	if(OldESP!=CurrentESP)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldESP!=CurrentESP)
	{
		ResetColor();
	}

	// EIP
    Print(REGISTER_WINDOW," EIP=");
	PICE_sprintf(tempDisplayRegs,"%.8X",CurrentEIP);
	if(OldEIP!=CurrentEIP)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldEIP!=CurrentEIP)
	{
		ResetColor();
	}

	// EFL
    Print(REGISTER_WINDOW," EFLAGS=");
	PICE_sprintf(tempDisplayRegs,"%.8X",CurrentEFL);
	if(OldEFL!=CurrentEFL)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldEFL!=CurrentEFL)
	{
		ResetColor();
	}

	// visual flags
	PICE_sprintf(tempDisplayRegs," %s\n",VisualizeFlags(CurrentEFL));
	Print(REGISTER_WINDOW,tempDisplayRegs);

	// CS
    Print(REGISTER_WINDOW,"CS=");
	PICE_sprintf(tempDisplayRegs,"%.4X",CurrentCS);
	if(OldCS!=CurrentCS)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldCS!=CurrentCS)
	{
		ResetColor();
	}

	// DS
    Print(REGISTER_WINDOW,"  DS=");
	PICE_sprintf(tempDisplayRegs,"%.4X",CurrentDS);
	if(OldDS!=CurrentDS)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldDS!=CurrentDS)
	{
		ResetColor();
	}

	// ES
    Print(REGISTER_WINDOW,"  ES=");
	PICE_sprintf(tempDisplayRegs,"%.4X",CurrentES);
	if(OldES!=CurrentES)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldES!=CurrentES)
	{
		ResetColor();
	}

	// FS
    Print(REGISTER_WINDOW,"  FS=");
	PICE_sprintf(tempDisplayRegs,"%.4X",CurrentFS);
	if(OldFS!=CurrentFS)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldFS!=CurrentFS)
	{
		ResetColor();
	}

	// GS
    Print(REGISTER_WINDOW,"  GS=");
	PICE_sprintf(tempDisplayRegs,"%.4X",CurrentGS);
	if(OldGS!=CurrentGS)
	{
		ResetColor();
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldGS!=CurrentGS)
	{
		ResetColor();
	}

	// SS
    Print(REGISTER_WINDOW,"  SS=");
	PICE_sprintf(tempDisplayRegs,"%.4X",CurrentSS);
	if(OldSS!=CurrentSS)
	{
		SetForegroundColor(COLOR_HILITE);
	}
	Print(REGISTER_WINDOW,tempDisplayRegs);
	if(OldSS!=CurrentSS)
	{
		ResetColor();
	}

    LEAVE_FUNC();
}

//*************************************************************************
// SaveOldRegs()
//
//*************************************************************************
void SaveOldRegs(void)
{

    ENTER_FUNC();

    OldEAX=CurrentEAX;
	OldEBX=CurrentEBX;
	OldECX=CurrentECX;
	OldEDX=CurrentEDX;
	OldESI=CurrentESI;
	OldEDI=CurrentEDI;
	OldEBP=CurrentEBP;
	OldESP=CurrentESP;
	OldEIP=CurrentEIP;
	OldEFL=CurrentEFL;
	OldCS=CurrentCS;
	OldDS=CurrentDS;
	OldES=CurrentES;
	OldFS=CurrentFS;
	OldGS=CurrentGS;
	OldSS=CurrentSS;

    LEAVE_FUNC();
}

//*************************************************************************
// GetKeyStatus()
//
//*************************************************************************
UCHAR GetKeyStatus(void)
{
    UCHAR ucRet;
    ucRet = inb_p(I8042_PHYSICAL_BASE + I8042_STATUS_REGISTER_OFFSET);
    return ucRet;
}

//*************************************************************************
// GetKeyData()
//
//*************************************************************************
UCHAR GetKeyData(void)
{
    UCHAR ucRet;
    ucRet = inb_p(I8042_PHYSICAL_BASE + I8042_DATA_REGISTER_OFFSET);
    return ucRet;
}

//*************************************************************************
// GetKeyPolled
//
//*************************************************************************
UCHAR KeyboardGetKeyPolled(void)
{
    UCHAR ucKey;
    UCHAR ucStatus;
    static BOOLEAN bExtended = FALSE;

    while(ucKey=0,(ucStatus=GetKeyStatus())&OUTPUT_BUFFER_FULL)
    {
        ucKey = 0;
        ucKey = GetKeyData();

        if(ucStatus&MOUSE_OUTPUT_BUFFER_FULL)
            continue;

        DPRINT((1,"GetKeyPolled(): key = %x bExtended=%s\n",ucKey,bExtended?"TRUE":"FALSE"));

        if(SCANCODE_EXTENDED == ucKey)
        {
            DPRINT((1,"extended switched ON\n"));
            bExtended = TRUE;
            continue;
        }
        else
        {
            if(!(ucKey&0x80)) // keypress
            {
				switch(ucKey&0x7f)
				{
					case SCANCODE_L_CTRL:
					case SCANCODE_R_CTRL:
						if(!bExtended)
							bControl=TRUE;
						break;
					case SCANCODE_L_SHIFT:
					case SCANCODE_R_SHIFT:
						if(!bExtended)
							bShift=TRUE;
						break;
					case SCANCODE_L_ALT:
					case SCANCODE_R_ALT:
	                    if(!bExtended)
		                    bAlt=TRUE;
						break;
					default:
				        DPRINT((0,"GetKeyPolled(): control = %u shift = %u alt = %u\n",bControl,bShift,bAlt));
						return ucKey;
                }
            }
		    else
		    {
				switch(ucKey&0x7f)
				{
					case SCANCODE_L_CTRL:
					case SCANCODE_R_CTRL:
						if(!bExtended)
							bControl=FALSE;
						break;
					case SCANCODE_L_SHIFT:
					case SCANCODE_R_SHIFT:
						if(!bExtended)
							bShift=FALSE;
						break;
					case SCANCODE_L_ALT:
					case SCANCODE_R_ALT:
	                    if(!bExtended)
		                    bAlt=FALSE;
						break;
                }
		    }
        }
        bExtended=FALSE;
    }

    return ucKey;
}

//*************************************************************************
// KeyboardFlushKeyboardQueue()
//
//*************************************************************************
void KeyboardFlushKeyboardQueue(void)
{
	__udelay(10);
    while(GetKeyStatus()&OUTPUT_BUFFER_FULL)
    {
        GetKeyData();
		__udelay(10);
    }
}

//*************************************************************************
// CheckLoadAbort()
//
//*************************************************************************
BOOLEAN CheckLoadAbort(void)
{
ULONG i;
UCHAR ucKey;

	MaskIrqs();

    SaveGraphicsState();

	FlushKeyboardQueue();

    PrintLogo(TRUE);

    for(i=0;i<5000;i++)
    {
		if(!(i%1000) )
		{
			PICE_sprintf(tempUtil,"\n LOAD WILL CONTINUE IN %u SEC (HIT 'C' TO CONTINUE OR ANY OTHER KEY TO ABORT)\n",5-i/1000);
			Clear(REGISTER_WINDOW);
			Print(REGISTER_WINDOW,tempUtil);
            PrintLogo(TRUE);
		}

        ucKey = GetKeyPolled();

        if(ucKey)
        {
            if((ucKey&0x7f)!=46)
            {
                RestoreGraphicsState();
				UnmaskIrqs();
				return FALSE;
            }
            else
                goto load;
        }
        __udelay(1000);
    }
load:
	Clear(REGISTER_WINDOW);
    PrintLogo(TRUE);

	tempUtil[0] = 0;
	FlushKeyboardQueue();

    RestoreGraphicsState();

	UnmaskIrqs();

    return TRUE;
}




//*************************************************************************
// IntelStackWalk()
//
//*************************************************************************
void IntelStackWalk(ULONG pc,ULONG ebp,ULONG esp)
{
    PULONG pFrame, pPrevFrame;
    LPSTR pSymbolName;

    DPRINT((0,"IntelStackWalk(): pc = %X ebp = %X esp = %X\n",pc,ebp,esp));

    pFrame = pPrevFrame = (PULONG)ebp;

    PutStatusText("EIP      FRAME    NAME\n");
    while(1)
    {
        DPRINT((0,"IntelStackWalk(): pFrame = %X pPrevFrame = %X pc =%X\n",(ULONG)pFrame,(ULONG)pPrevFrame,pc));
        if ( ( (ULONG)pFrame & 3 )    ||
             ( (pFrame <= pPrevFrame) ) )
        {
            DPRINT((0,"IntelStackWalk(): pFrame is either unaligned or not less than previous\n"));
            if( !IsRangeValid((ULONG)pFrame, sizeof(PVOID)*2) )
            {
                DPRINT((0,"IntelStackWalk(): pFrame not valid pointer!\n"));
                break;
            }
        }

        if((pSymbolName = FindFunctionByAddress(pc,NULL,NULL)) )
		    PICE_sprintf(tempUtil,"%08X %08X %s\n",pc, (ULONG)pFrame,pSymbolName);
		else
		    PICE_sprintf(tempUtil,"%08X %08X\n",pc, (ULONG)pFrame);
        Print(OUTPUT_WINDOW,tempUtil);
        if(WaitForKey()==FALSE)break;

        pc = pFrame[1];

        pPrevFrame = pFrame;

        pFrame = (PULONG)pFrame[0]; // proceed to next higher frame on stack
    }
}

//*************************************************************************
// FindPteForLinearAddress()
//
//*************************************************************************
pte_t * FindPteForLinearAddress(ULONG address)
{
	pgd_t * pPGD;
	pmd_t * pPMD;
	pte_t * pPTE;
	struct mm_struct* p = NULL;
	struct task_struct* my_current = (struct task_struct*)0xFFFFE000;

    ENTER_FUNC();

	address &= (~(PAGE_SIZE-1));

    // if we're in DebuggerShell() we live on a different stack
    if(bInDebuggerShell)
	    (ULONG)my_current &= ulRealStackPtr;
    else
	    my_current = current;

    if(address < TASK_SIZE)
	{
		p = my_current->mm;
	}
    else
    {
        p = my_init_mm;
    }

	if(p)
	{
		pPGD = pgd_offset(p,address);
        if(pPGD && pgd_val(*pPGD)&_PAGE_PRESENT)
        {
            // not large page
            if(!(pgd_val(*pPGD)&_PAGE_4M))
			{
				pPMD = pmd_offset(pPGD,address);
				if(pPMD)
				{
					pPTE = pte_offset(pPMD,address);
					if(pPTE)
					{
                        LEAVE_FUNC();
						return pPTE;
					}
				}
			}
			// large page
			else
			{
                LEAVE_FUNC();
				return NULL;
			}
		}
	}

    LEAVE_FUNC();
	return NULL;
}

//*************************************************************************
// InvalidateLB()
//
//*************************************************************************
void InvalidateLB(void)
{
	ENTER_FUNC();
    __asm__ __volatile__
	(
		"wbinvd
		mov %%cr3,%%ecx
        mov %%ecx,%%cr3"
        :::"ecx"
    );
	LEAVE_FUNC();
}

//*************************************************************************
// ReadPhysMem()
//
//*************************************************************************
ULONG ReadPhysMem(ULONG Address,ULONG ulSize)
{
    ULONG Page = ((ULONG)TwoPagesForPhysMem+PAGE_SIZE)&~(PAGE_SIZE-1);
    PULONG pPTE;
	ULONG temp = 0;
	ULONG oldPTE;

    ENTER_FUNC();
    DPRINT((0,"ReadPhysMem(%.8X,%u)\n",Address,ulSize));
    DPRINT((0,"ReadPhysMem(): Page = %.8X\n",Page));
    pPTE = (PULONG)FindPteForLinearAddress(Page);
    DPRINT((0,"ReadPhysMem(): pPTE = %.8X\n",pPTE));
	if(pPTE)
	{
		oldPTE = *pPTE;
        DPRINT((0,"ReadPhysMem(): oldPTE = %.8X\n",oldPTE));
		temp = (Address & ~(PAGE_SIZE-1));
        DPRINT((0,"ReadPhysMem(): page-aligned Address = %.8X\n",temp));
		*pPTE = temp|0x1;
        DPRINT((0,"ReadPhysMem(): new PTE = %.8X\n",*pPTE));
        InvalidateLB();
		switch(ulSize)
		{
			case sizeof(UCHAR): // BYTE
				temp = *(PUCHAR)(Page + (Address & (PAGE_SIZE-1)));
				temp = (UCHAR)temp;
				break;
			case sizeof(USHORT): // WORD
				temp = *(PUSHORT)(Page + (Address & (PAGE_SIZE-1)));
				temp = (USHORT)temp;
				break;
			case sizeof(ULONG): // DWORD
				temp = *(PULONG)(Page + (Address & (PAGE_SIZE-1)));
				break;
		}
		*pPTE = oldPTE;
        InvalidateLB();
	}
    LEAVE_FUNC();

    return temp;
}

//*************************************************************************
// WritePhysMem()
//
//*************************************************************************
void WritePhysMem(ULONG Address,ULONG Datum,ULONG ulSize)
{
    ULONG Page = ((ULONG)TwoPagesForPhysMem+PAGE_SIZE)&~(PAGE_SIZE-1);
    PULONG pPTE;
	ULONG temp;
	ULONG oldPTE;

    pPTE = (PULONG)FindPteForLinearAddress(Page);
	if(pPTE)
	{
		oldPTE = *pPTE;
		temp = (Address & ~(PAGE_SIZE-1));
		*pPTE = temp | 0x3; // present and writable
        InvalidateLB();
		switch(ulSize)
		{
			case sizeof(UCHAR): // BYTE
				*(PUCHAR)(Page + (Address & (PAGE_SIZE-1))) = (UCHAR)Datum;
				break;
			case sizeof(USHORT): // WORD
				*(PUSHORT)(Page + (Address & (PAGE_SIZE-1))) = (USHORT)Datum;
				break;
			case sizeof(ULONG): // DWORD
				*(PULONG)(Page + (Address & (PAGE_SIZE-1))) = Datum;
				break;
		}
		*pPTE = oldPTE;
        InvalidateLB();
	}
}

/////////////////////////////////////////////////////////////////////////////
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((*cp == 'x') && isxdigit(cp[1])) {
				cp++;
				base = 16;
			}
		}
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

long simple_strtol(const char *cp,char **endp,unsigned int base)
{
	if(*cp=='-')
		return -simple_strtoul(cp+1,endp,base);
	return simple_strtoul(cp,endp,base);
}

/* we use this so that we can do without the ctype library */
#define is_digit(c)	((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s)
{
	int i=0;

	while (is_digit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

#define NUM_ZEROPAD	1		/* pad with zero */
#define NUM_SIGN	2		/* unsigned/signed long */
#define NUM_PLUS	4		/* show plus */
#define NUM_SPACE	8		/* space if plus */
#define NUM_LEFT	16		/* left justified */
#define NUM_SPECIAL	32		/* 0x */
#define NUM_LARGE	64		/* use 'ABCDEF' instead of 'abcdef' */

#define do_div(n,base) ({ \
int __res; \
__res = ((unsigned long) n) % (unsigned) base; \
n = ((unsigned long) n) / (unsigned) base; \
__res; })

static char * PICE_number(char * str, long num, int base, int size, int precision
	,int type)
{
	char c,sign,tmp[66];
	const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
	int i;

	if (type & NUM_LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & NUM_LEFT)
		type &= ~NUM_ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & NUM_ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & NUM_SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		} else if (type & NUM_PLUS) {
			sign = '+';
			size--;
		} else if (type & NUM_SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (type & NUM_SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num != 0)
		tmp[i++] = digits[do_div(num,base)];
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type&(NUM_ZEROPAD+NUM_LEFT)))
		while(size-->0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type & NUM_SPECIAL) {
		if (base==8)
			*str++ = '0';
		else if (base==16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}
	if (!(type & NUM_LEFT))
		while (size-- > 0)
			*str++ = c;
	while (i < precision--)
		*str++ = '0';
	while (i-- > 0)
		*str++ = tmp[i];
	while (size-- > 0)
		*str++ = ' ';
	return str;
}

/* Forward decl. needed for IP address printing stuff... */
int PICE_sprintf(char * buf, const char *fmt, ...);

int PICE_vsprintf(char *buf, const char *fmt, va_list args)
{
	int len;
	unsigned long num;
	int i, base;
	char * str;
	const char *s;

	int flags;		/* flags to PICE_number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				   PICE_number of chars for from string */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

	for (str=buf ; *fmt ; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case '-': flags |= NUM_LEFT; goto repeat;
				case '+': flags |= NUM_PLUS; goto repeat;
				case ' ': flags |= NUM_SPACE; goto repeat;
				case '#': flags |= NUM_SPECIAL; goto repeat;
				case '0': flags |= NUM_ZEROPAD; goto repeat;
				}

		/* get field width */
		field_width = -1;
		if (is_digit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= NUM_LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		/* default base */
		base = 10;

		switch (*fmt) {
		case 'c':
			if (!(flags & NUM_LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			continue;

		case 's':
			s = va_arg(args, char *);
			if (!s)
				s = "<NULL>";

			len = strnlen(s, precision);

			if (!(flags & NUM_LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			continue;

		case 'p':
			if (field_width == -1) {
				field_width = 2*sizeof(void *);
				flags |= NUM_ZEROPAD;
			}
			str = PICE_number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			continue;


		case 'n':
			if (qualifier == 'l') {
				long * ip = va_arg(args, long *);
				*ip = (str - buf);
			} else {
				int * ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			continue;

		case '%':
			*str++ = '%';
			continue;

		/* integer PICE_number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;

		case 'X':
			flags |= NUM_LARGE;
		case 'x':
			base = 16;
			break;

		case 'd':
		case 'i':
			flags |= NUM_SIGN;
		case 'u':
			break;

		default:
			*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			continue;
		}
		if (qualifier == 'l')
			num = va_arg(args, unsigned long);
		else if (qualifier == 'h') {
			num = (unsigned short) va_arg(args, int);
			if (flags & NUM_SIGN)
				num = (short) num;
		} else if (flags & NUM_SIGN)
			num = va_arg(args, int);
		else
			num = va_arg(args, unsigned int);
		str = PICE_number(str, num, base, field_width, precision, flags);
	}
	*str = '\0';
	return str-buf;
}

int PICE_sprintf(char * buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = PICE_vsprintf(buf,fmt,args);
	va_end(args);
	return i;
}

//*************************************************************************
// AsciiFromScan()
//
// Convert Scancode to ASCII
//*************************************************************************
UCHAR AsciiFromScan(UCHAR s)
{
    ULONG i;

    ENTER_FUNC();

	if(!bShift)
	{
        if(ucKeyboardLayout == GERMANY)
        {
		    for(i=0;i<DIM(ucScanToAscii_GER);i++)
		    {
			    if(ucScanToAscii_GER[i].s==s)
                {
                    LEAVE_FUNC();
				    return ucScanToAscii_GER[i].a;
                }
		    }
        }
        else
        {
		    for(i=0;i<DIM(ucScanToAscii_US);i++)
		    {
			    if(ucScanToAscii_US[i].s==s)
                {
                    LEAVE_FUNC();
				    return ucScanToAscii_US[i].a;
                }
		    }
        }
	}
	else
	{
        if(ucKeyboardLayout == GERMANY)
        {
		    for(i=0;i<DIM(ucShiftScanToAscii_GER);i++)
		    {
			    if(ucShiftScanToAscii_GER[i].s==s)
                {
                    LEAVE_FUNC();
				    return ucShiftScanToAscii_GER[i].a;
                }
		    }
        }
        else
        {
		    for(i=0;i<DIM(ucShiftScanToAscii_US);i++)
		    {
			    if(ucShiftScanToAscii_GER[i].s==s)
                {
                    LEAVE_FUNC();
				    return ucShiftScanToAscii_GER[i].a;
                }
		    }
        }
	}
    DPRINT((0,"AsciiFromScan(): no translation for key\n"));
    LEAVE_FUNC();
	return 0;
}


//*************************************************************************
// AsciiToScan()
//
// Convert Scancode to ASCII
//*************************************************************************
UCHAR AsciiToScan(UCHAR s)
{
    ULONG i;

    ENTER_FUNC();

    if(ucKeyboardLayout == GERMANY)
    {
		for(i=0;i<DIM(ucShiftScanToAscii_GER);i++)
		{
			if(ucShiftScanToAscii_GER[i].a==s)
            {
                LEAVE_FUNC();
				return ucShiftScanToAscii_GER[i].s;
            }
		}
    }
    else
    {
		for(i=0;i<DIM(ucShiftScanToAscii_US);i++)
		{
			if(ucShiftScanToAscii_GER[i].a==s)
            {
                LEAVE_FUNC();
				return ucShiftScanToAscii_GER[i].s;
            }
		}
    }

    DPRINT((0,"AsciiToScan(): no translation for ASCII code\n"));
    LEAVE_FUNC();
	return 0;
}

//************************************************************************
// outportb()
//
//************************************************************************
void outportb(USHORT port,UCHAR data)
{
    WRITE_PORT_UCHAR(port, data);
}

void outb_p(UCHAR data, USHORT port)
{
	WRITE_PORT_UCHAR(port, data);
}

VOID  outl(ULONG data, PULONG port)
{
	WRITE_PORT_ULONG(port, data);
}


//************************************************************************
// inportb()
//
//************************************************************************
UCHAR inportb(USHORT port)
{
    return READ_PORT_UCHAR(port);
}

UCHAR inb_p(USHORT port)
{
    return READ_PORT_UCHAR(port);
}

ULONG  inl(PULONG port)
{
	return READ_PORT_ULONG(port);
}


//*************************************************************************
// GetInitMm()
//
//*************************************************************************
struct mm_struct *GetInitMm(void)
{

#if REAL_LINUX_VERSION_CODE >= 0x02020B
    ENTER_FUNC();
    /* symbol export of init_mm was added in 2.2.11 */
    LEAVE_FUNC();
    return &init_mm;
#else
    // see also Rubini, Linux Device Drivers, page 288
	struct task_struct *pt;

    ENTER_FUNC();

	for (pt = current->next_task; pt != current; pt = pt->next_task) {
		if (pt->pid == 0) {
            LEAVE_FUNC();
			return pt->mm;
		}
	}

    DPRINT((0,"GetInitMm(): failure\n"));
    LEAVE_FUNC();
	return NULL;
#endif
}

//*************************************************************************
// EnablePassThrough()
//
// enable MDA passthrough on AGP chipset
//*************************************************************************
void EnablePassThrough(void)
{
	ULONG oldCF8,flags;

	save_flags(flags);
	cli();

	oldCF8 = inl(0xcf8);
	outl(0x80000050,0xcf8);
	outl(inl(0xcfc)|0x00000020,0xcfc);
	outl(oldCF8,0xcf8);

	restore_flags(flags);
}

//***********************************************************************************
//	Pice_malloc - allocate memory from paged or non-paged pool
//***********************************************************************************
void * PICE_malloc( size_t numBytes, BOOLEAN fromPaged )
{
	void* res = ExAllocatePool( (fromPaged)?PagedPool:NonPagedPool, numBytes );
	assert(res);
	return res;
}

//***********************************************************************************
//	PICE_free - free memory allocated by PICE_malloc
//***********************************************************************************
void PICE_free( void* p )
{
	assert( p );
	ExFreePool( p );
}

long PICE_read(HANDLE hFile, LPVOID lpBuffer, long lBytes)
{
	DWORD	NumberOfBytesRead;
	IO_STATUS_BLOCK  iosb;

	assert( lpBuffer );

	if (!NT_SUCCESS(NtReadFile(
		(HANDLE) hFile,
		NULL, NULL, NULL, &iosb,
		(LPVOID) lpBuffer,
		(DWORD) lBytes,
		NULL,
		NULL
		)))
	{
		return -1;
	}
	NumberOfBytesRead = iosb.Information;
	return NumberOfBytesRead;
}

HANDLE PICE_open (LPCWSTR	lpPathName,	int	iReadWrite)
{
	DWORD dwAccessMask = 0;
	DWORD dwShareMode = 0;
	UNICODE_STRING TmpFileName;
 	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE hfile;
	NTSTATUS status;

	if ( (iReadWrite & OF_READWRITE ) == OF_READWRITE )
		dwAccessMask = GENERIC_READ | GENERIC_WRITE;
	else if ( (iReadWrite & OF_READ ) == OF_READ )
		dwAccessMask = GENERIC_READ;
	else if ( (iReadWrite & OF_WRITE ) == OF_WRITE )
		dwAccessMask = GENERIC_WRITE;

	if ((iReadWrite & OF_SHARE_COMPAT) == OF_SHARE_COMPAT )
		dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE;
	else if ((iReadWrite & OF_SHARE_DENY_NONE) == OF_SHARE_DENY_NONE)
		dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE;
	else if ((iReadWrite & OF_SHARE_DENY_READ) == OF_SHARE_DENY_READ)
		dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	else if ((iReadWrite & OF_SHARE_DENY_WRITE) == OF_SHARE_DENY_WRITE )
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
	else if ((iReadWrite & OF_SHARE_EXCLUSIVE) == OF_SHARE_EXCLUSIVE)
		dwShareMode = 0;

	RtlInitUnicodeString (&TmpFileName, lpPathName);
  	InitializeObjectAttributes(&ObjectAttributes,
                             &TmpFileName,
                             0,
                             NULL,
                             NULL);


	status = NtOpenFile( &hfile,
                      dwAccessMask,
                      &ObjectAttributes,
                      NULL, dwShareMode, 0);
	//BUG BUG check status!!!
	return hfile;
}

int PICE_close (HANDLE	hFile)
{
	if (NT_SUCCESS( ZwClose((HANDLE)hFile)))
	{
		return 0;
	}
	DbgPrint("ZwClose failed:\n");
	return -1;
}

size_t PICE_len( HANDLE hFile )
{
	FILE_STANDARD_INFORMATION fs;
	IO_STATUS_BLOCK  iosb;
	NTSTATUS status;

  	status = ZwQueryInformationFile( hFile, &iosb, &fs, sizeof fs, FileStandardInformation );
	assert(fs.EndOfFile.HighPart == 0);
	return (size_t)fs.EndOfFile.LowPart;
}

