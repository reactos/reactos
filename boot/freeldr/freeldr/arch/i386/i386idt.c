
#include <freeldr.h>


KIDTENTRY DECLSPEC_ALIGN(4) i386Idt[32];
KDESCRIPTOR i386IdtDescriptor = {0, 255, (ULONG)i386Idt};

static
void
InitIdtVector(
    UCHAR Vector,
    PVOID ServiceHandler,
    USHORT Access)
{
    i386Idt[Vector].Offset = (ULONG)ServiceHandler & 0xffff;
    i386Idt[Vector].ExtendedOffset = (ULONG)ServiceHandler >> 16;
    i386Idt[Vector].Selector = PMODE_CS;
    i386Idt[Vector].Access = Access;
}

void
__cdecl
InitIdt(void)
{
    InitIdtVector(0, i386DivideByZero, 0x8e00);
    InitIdtVector(1, i386DebugException, 0x8e00);
    InitIdtVector(2, i386NMIException, 0x8e00);
    InitIdtVector(3, i386Breakpoint, 0x8e00);
    InitIdtVector(4, i386Overflow, 0x8e00);
    InitIdtVector(5, i386BoundException, 0x8e00);
    InitIdtVector(6, i386InvalidOpcode, 0x8e00);
    InitIdtVector(7, i386FPUNotAvailable, 0x8e00);
    InitIdtVector(8, i386DoubleFault, 0x8e00);
    InitIdtVector(9, i386CoprocessorSegment, 0x8e00);
    InitIdtVector(10, i386InvalidTSS, 0x8e00);
    InitIdtVector(11, i386SegmentNotPresent, 0x8e00);
    InitIdtVector(12, i386StackException, 0x8e00);
    InitIdtVector(13, i386GeneralProtectionFault, 0x8e00);
    InitIdtVector(14, i386PageFault, 0x8e00);
    InitIdtVector(16, i386CoprocessorError, 0x8e00);
    InitIdtVector(17, i386AlignmentCheck, 0x8e00);
    InitIdtVector(18, i386MachineCheck, 0x8e00);
}
