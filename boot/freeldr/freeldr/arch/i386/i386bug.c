
#include <freeldr.h>

#include <debug.h>

typedef struct _FRAME
{
    struct _FRAME *Next;
    void *Address;
} FRAME;

static const CHAR *i386ExceptionDescriptionText[] =
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

#define SCREEN_ATTR 0x1F    // Bright white on blue background

/* Used to store the current X and Y position on the screen */
static ULONG i386_ScreenPosX = 0;
static ULONG i386_ScreenPosY = 0;

#if 0
static void
i386PrintChar(CHAR chr, ULONG x, ULONG y)
{
    MachVideoPutChar(chr, SCREEN_ATTR, x, y);
}
#endif

static void
i386PrintText(CHAR *pszText)
{
    ULONG Width, Unused;

    MachVideoGetDisplaySize(&Width, &Unused, &Unused);

    for (; *pszText != ANSI_NULL; ++pszText)
    {
        if (*pszText == '\n')
        {
            i386_ScreenPosX = 0;
            ++i386_ScreenPosY;
            continue;
        }

        MachVideoPutChar(*pszText, SCREEN_ATTR, i386_ScreenPosX, i386_ScreenPosY);
        if (++i386_ScreenPosX >= Width)
        {
            i386_ScreenPosX = 0;
            ++i386_ScreenPosY;
        }
    // FIXME: Implement vertical screen scrolling if we are at the end of the screen.
    }
}

static void
PrintTextV(const CHAR *Format, va_list args)
{
    CHAR Buffer[512];

    _vsnprintf(Buffer, sizeof(Buffer), Format, args);
    Buffer[sizeof(Buffer) - 1] = ANSI_NULL;

    i386PrintText(Buffer);
}

static void
PrintText(const CHAR *Format, ...)
{
    va_list argptr;

    va_start(argptr, Format);
    PrintTextV(Format, argptr);
    va_end(argptr);
}

static void
i386PrintFrames(PKTRAP_FRAME TrapFrame)
{
    FRAME *Frame;

    PrintText("Frames:\n");
#ifdef _M_IX86
    for (Frame = (FRAME*)TrapFrame->Ebp;
         Frame != 0 && (ULONG_PTR)Frame < STACKADDR;
         Frame = Frame->Next)
#else
    for (Frame = (FRAME*)TrapFrame->TrapFrame;
         Frame != 0 && (ULONG_PTR)Frame < STACKADDR;
         Frame = Frame->Next)
#endif
    {
        PrintText("%p  ", Frame->Address);
    }
}

void
NTAPI
i386PrintExceptionText(ULONG TrapIndex, PKTRAP_FRAME TrapFrame, PKSPECIAL_REGISTERS Special)
{
    PUCHAR InstructionPointer;

    MachVideoHideShowTextCursor(FALSE);
    MachVideoClearScreen(SCREEN_ATTR);
    i386_ScreenPosX = 0;
    i386_ScreenPosY = 0;

    PrintText("An error occured in " VERSION "\n"
              "Report this error on the ReactOS Bug Tracker: https://jira.reactos.org\n\n"
              "0x%02lx: %s\n", TrapIndex, i386ExceptionDescriptionText[TrapIndex]);
#ifdef _M_IX86
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
              TrapFrame->SegDs, TrapFrame->ErrCode);
    PrintText("ES: %.4lx        EFLAGS: %.8lx\n",
              TrapFrame->SegEs, TrapFrame->EFlags);
    PrintText("FS: %.4lx        GDTR Base: %.8lx Limit: %.4x\n",
              TrapFrame->SegFs, Special->Gdtr.Base, Special->Gdtr.Limit);
    PrintText("GS: %.4lx        IDTR Base: %.8lx Limit: %.4x\n",
              TrapFrame->SegGs, Special->Idtr.Base, Special->Idtr.Limit);
    PrintText("SS: %.4lx        LDTR: %.4lx TR: %.4lx\n\n",
              TrapFrame->HardwareSegSs, Special->Ldtr, Special->Idtr.Limit);

    i386PrintFrames(TrapFrame);                        // Display frames
    InstructionPointer = (PUCHAR)TrapFrame->Eip;
#else
    PrintText("RAX: %.8lx        R8:  %.8lx        R12: %.8lx        RSI: %.8lx\n",
              TrapFrame->Rax, TrapFrame->R8, 0, TrapFrame->Rsi);
    PrintText("RBX: %.8lx        R9:  %.8lx        R13: %.8lx        RDI: %.8lx\n",
              TrapFrame->Rbx, TrapFrame->R9, 0, TrapFrame->Rdi);
    PrintText("RCX: %.8lx        R10: %.8lx        R14: %.8lx        RBP: %.8lx\n",
              TrapFrame->Rcx, TrapFrame->R10, 0, TrapFrame->Rbp);
    PrintText("RDX: %.8lx        R11: %.8lx        R15: %.8lx        RSP: %.8lx\n",
              TrapFrame->Rdx, TrapFrame->R11, 0, TrapFrame->Rsp);

    PrintText("CS: %.4lx        RIP: %.8lx\n",
              TrapFrame->SegCs, TrapFrame->Rip);
    PrintText("DS: %.4lx        ERROR CODE: %.8lx\n",
              TrapFrame->SegDs, TrapFrame->ErrorCode);
    PrintText("ES: %.4lx        EFLAGS: %.8lx\n",
              TrapFrame->SegEs, TrapFrame->EFlags);
    PrintText("FS: %.4lx        GDTR Base: %.8lx Limit: %.4x\n",
              TrapFrame->SegFs, Special->Gdtr.Base, Special->Gdtr.Limit);
    PrintText("GS: %.4lx        IDTR Base: %.8lx Limit: %.4x\n",
              TrapFrame->SegGs, Special->Idtr.Base, Special->Idtr.Limit);
    PrintText("SS: %.4lx        LDTR: %.4lx TR: %.4lx\n\n",
              TrapFrame->SegSs, Special->Ldtr, Special->Idtr.Limit);
    InstructionPointer = (PUCHAR)TrapFrame->Rip;
#endif
    PrintText("\nInstruction stream: %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x \n",
              InstructionPointer[0], InstructionPointer[1],
              InstructionPointer[2], InstructionPointer[3],
              InstructionPointer[4], InstructionPointer[5],
              InstructionPointer[6], InstructionPointer[7]);
}

VOID
FrLdrBugCheckWithMessage(
    ULONG BugCode,
    PCHAR File,
    ULONG Line,
    PSTR Format,
    ...)
{
    va_list argptr;

    MachVideoHideShowTextCursor(FALSE);
    MachVideoClearScreen(SCREEN_ATTR);
    i386_ScreenPosX = 0;
    i386_ScreenPosY = 0;

    PrintText("A problem has been detected and FreeLoader boot has been aborted.\n\n");

    PrintText("%ld: %s\n\n", BugCode, BugCodeStrings[BugCode]);

    if (File)
    {
        PrintText("Location: %s:%ld\n\n", File, Line);
    }

    va_start(argptr, Format);
    PrintTextV(Format, argptr);
    va_end(argptr);

    _disable();
    __halt();
    for (;;);
}

void
NTAPI
FrLdrBugCheckEx(
    ULONG BugCode,
    PCHAR File,
    ULONG Line)
{
    MachVideoHideShowTextCursor(FALSE);
    MachVideoClearScreen(SCREEN_ATTR);
    i386_ScreenPosX = 0;
    i386_ScreenPosY = 0;

    PrintText("A problem has been detected and FreeLoader boot has been aborted.\n\n");

    PrintText("%ld: %s\n\n", BugCode, BugCodeStrings[BugCode]);

    if (File)
    {
        PrintText("Location: %s:%ld\n\n", File, Line);
    }

    PrintText("Bug Information:\n    %p\n    %p\n    %p\n    %p\n    %p\n\n",
              BugCheckInfo[0], BugCheckInfo[1], BugCheckInfo[2], BugCheckInfo[3], BugCheckInfo[4]);

    _disable();
    __halt();
    for (;;);
}

void
NTAPI
FrLdrBugCheck(ULONG BugCode)
{
    FrLdrBugCheckEx(BugCode, 0, 0);
}
