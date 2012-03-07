
#include <freeldr.h>

#define NDEBUG
#include <debug.h>

typedef struct _FRAME
{
    struct _FRAME *Next;
    void *Address;
} FRAME;

char *i386ExceptionDescriptionText[] =
{
    "Exception 00: DIVIDE BY ZERO\n\n",
	"Exception 01: DEBUG EXCEPTION\n\n",
	"Exception 02: NON-MASKABLE INTERRUPT EXCEPTION\n\n",
	"Exception 03: BREAKPOINT (INT 3)\n\n",
	"Exception 04: OVERFLOW\n\n",
	"Exception 05: BOUND EXCEPTION\n\n",
	"Exception 06: INVALID OPCODE\n\n",
	"Exception 07: FPU NOT AVAILABLE\n\n",
	"Exception 08: DOUBLE FAULT\n\n",
	"Exception 09: COPROCESSOR SEGMENT OVERRUN\n\n",
	"Exception 0A: INVALID TSS\n\n",
	"Exception 0B: SEGMENT NOT PRESENT\n\n",
	"Exception 0C: STACK EXCEPTION\n\n",
	"Exception 0D: GENERAL PROTECTION FAULT\n\n",
	"Exception 0E: PAGE FAULT\n\n",
	"Exception 0F: Reserved\n\n",
	"Exception 10: COPROCESSOR ERROR\n\n",
	"Exception 11: ALIGNMENT CHECK\n\n",
	"Exception 12: MACHINE CHECK\n\n"
};

#define SCREEN_ATTR 0x1f
void
i386PrintChar(char chr, ULONG x, ULONG y)
{
	MachVideoPutChar(chr, SCREEN_ATTR, x, y);
}

/* Used to store the current X and Y position on the screen */
ULONG i386_ScreenPosX = 0;
ULONG i386_ScreenPosY = 0;

void
i386PrintText(char *pszText)
{
    char chr;
    while (1)
    {
        chr = *pszText++;
        
        if (chr == 0) break;
        if (chr == '\n')
        {
            i386_ScreenPosY++;
            i386_ScreenPosX = 0;
            continue;
        }
        
        MachVideoPutChar(chr, SCREEN_ATTR, i386_ScreenPosX, i386_ScreenPosY);
        i386_ScreenPosX++;
    }
}

void
PrintText(const char *format, ...)
{
    va_list argptr;
    char buffer[256];
    
    va_start(argptr, format);
    _vsnprintf(buffer, sizeof(buffer), format, argptr);
    buffer[sizeof(buffer) - 1] = 0;
    va_end(argptr);
    i386PrintText(buffer);
}

void
i386PrintFrames(PKTRAP_FRAME TrapFrame)
{
    FRAME *Frame;

    PrintText("Frames:\n");

    for (Frame = (FRAME*)TrapFrame->Ebp;
         Frame != 0 && (ULONG_PTR)Frame < STACK32ADDR;
         Frame = Frame->Next)
    {
        PrintText("%p  ", Frame->Address);
    }
}

void
NTAPI
i386PrintExceptionText(ULONG TrapIndex, PKTRAP_FRAME TrapFrame, PKSPECIAL_REGISTERS Special)
{
	MachVideoClearScreen(SCREEN_ATTR);
	i386_ScreenPosX = 0;
	i386_ScreenPosY = 0;

    PrintText("An error occured in FreeLoader\n"
              VERSION"\n"
	          "Report this error to the ReactOS Development mailing list <ros-dev@reactos.org>\n\n"
              "%s\n", i386ExceptionDescriptionText[TrapIndex]);

    PrintText("EAX: %.8lx        ESP: %.8lx        CR0: %.8lx        DR0: %.8lx\n", 
              TrapFrame->Eax, TrapFrame->HardwareEsp, Special->Cr0, TrapFrame->Dr0);
    PrintText("EBX: %.8lx        EBP: %.8lx        CR1: ????????        DR1: %.8lx\n", 
              TrapFrame->Ebx, TrapFrame->Ebp, TrapFrame->Dr1);
    PrintText("ECX: %.8lx        ESI: %.8lx        CR2: %.8lx        DR2: %.8lx\n", 
              TrapFrame->Ecx, TrapFrame->Esi, Special->Cr2, TrapFrame->Dr2);
    PrintText("EDX: %.8lx        EDI: %.8lx        CR3: %.8lx        DR3: %.8lx\n", 
              TrapFrame->Edx, TrapFrame->Edi, Special->Cr3, TrapFrame->Dr3);
    PrintText("                                                               DR6: %.8lx\n", 
              TrapFrame->Dr6);
    PrintText("                                                               DR7: %.8lx\n\n", 
              TrapFrame->Dr7);
    PrintText("CS: %.4lx        EIP: %.8lx\n", 
              TrapFrame->SegCs, TrapFrame->Eip);
    PrintText("DS: %.4lx        ERROR CODE: %.8lx\n", 
              TrapFrame->SegDs, TrapFrame->Eip);
    PrintText("ES: %.4lx        EFLAGS: %.8lx\n", 
              TrapFrame->SegEs, TrapFrame->EFlags);
    PrintText("FS: %.4lx        GDTR Base: %.8lx Limit: %.4x\n", 
              TrapFrame->SegFs, Special->Gdtr.Base, Special->Gdtr.Limit);
    PrintText("GS: %.4lx        IDTR Base: %.8lx Limit: %.4x\n", 
              TrapFrame->SegGs, Special->Idtr.Base, Special->Idtr.Limit);
    PrintText("SS: %.4lx        LDTR: %.4lx TR: %.4lx\n\n", 
              TrapFrame->HardwareSegSs, Special->Ldtr, Special->Idtr.Limit);

	i386PrintFrames(TrapFrame);						// Display frames
}
