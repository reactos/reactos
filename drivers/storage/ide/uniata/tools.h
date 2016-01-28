/*++

Copyright (c) 2002-2012 Alexandr A. Telyatnikov (Alter)

Module Name:
    tools.h

Abstract:
    This header contains some useful definitions for data manipulation.

Author:
    Alexander A. Telyatnikov (Alter)

Environment:
    kernel mode only

Notes:

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Revision History:

--*/

#ifndef __TOOLS_H__
#define __TOOLS_H__

#pragma pack(push, 1)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//----------------

#ifndef USER_MODE
#include <ntddk.h>                  // various NT definitions
#endif //USER_MODE

//----------------

#ifndef FOUR_BYTE_DEFINED
#define FOUR_BYTE_DEFINED
typedef struct _FOUR_BYTE {
    UCHAR Byte0;
    UCHAR Byte1;
    UCHAR Byte2;
    UCHAR Byte3;
} FOUR_BYTE, *PFOUR_BYTE;
#endif //FOUR_BYTE_DEFINED

#ifdef _DEBUG

#ifndef DBG
#define DBG
#endif // DBG

#else // _DEBUG

#ifdef DBG
#undef DBG
#endif // DBG

#endif // _DEBUG

// This macro has the effect of Bit = log2(Data)

#define WHICH_BIT(Data, Bit) {                      \
    for (Bit = 0; Bit < 32; Bit++) {                \
        if ((Data >> Bit) == 1) {                   \
            break;                                  \
        }                                           \
    }                                               \
}

#define PAGE_MASK (PAGE_SIZE-1)

#define ntohs(x)    ( (((USHORT)x[0])<<8) | x[1]  )
#define PLAY_ACTIVE(DeviceExtension) (((PCDROM_DATA)(DeviceExtension + 1))->PlayActive)
#define MSF_TO_LBA(Minutes,Seconds,Frames) \
                (ULONG)((60 * 75 * (Minutes)) + (75 * (Seconds)) + ((Frames) - 150))
#define LBA_TO_MSF(Lba,Minutes,Seconds,Frames)               \
{                                                            \
    (Minutes) = (UCHAR)(Lba  / (60 * 75));                   \
    (Seconds) = (UCHAR)((Lba % (60 * 75)) / 75);             \
    (Frames)  = (UCHAR)((Lba % (60 * 75)) % 75);             \
}
#define DEC_TO_BCD(x) (((x / 10) << 4) + (x % 10))


#ifdef DBG

#define KdDump(a,b)                         \
if((a)!=NULL) {                             \
    ULONG i;                                \
    for(i=0; i<(b); i++) {                  \
        ULONG c;                            \
        c = (ULONG)(*(((PUCHAR)(a))+i));    \
        KdPrint(("%2.2x ",c));              \
        if ((i & 0x0f) == 0x0f) KdPrint(("\n"));   \
    }                                       \
    KdPrint(("\n"));                        \
}

#define DbgDumpRegTranslation(chan, idx) \
        KdPrint2((PRINT_PREFIX \
                   "  IO_%#x (%#x), %s:\n", \
                   idx,          \
                   chan->RegTranslation[idx].Addr, \
                   chan->RegTranslation[idx].Proc ? "Proc" : (        \
                   chan->RegTranslation[idx].MemIo ? "Mem" : "IO"))); \

#define BrutePoint() { ASSERT(0); }

#define DbgAllocatePool(x,y) ExAllocatePool(x,y)
#define DbgFreePool(x) ExFreePool(x)
#define DbgAllocatePoolWithTag(a,b,c) ExAllocatePoolWithTag(a,b,c)

#else

#define KdDump(a,b) {}

#define DbgDumpRegTranslation(chan, idx) {}

#define DbgAllocatePool(x,y) ExAllocatePool(x,y)
#define DbgFreePool(x) ExFreePool(x)
#define DbgAllocatePoolWithTag(a,b,c) ExAllocatePoolWithTag(a,b,c)

#define BrutePoint() {}

#endif //DBG



#define WAIT_FOR_XXX_EMU_DELAY  DEF_I64(5000000)        //  0.5 s

// neat little hacks to count number of bits set efficiently
__inline ULONG CountOfSetBitsUChar(UCHAR _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
__inline ULONG CountOfSetBitsULong(ULONG _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif //max

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif //min
//#define abs(a) (((a) < 0) ? (-(a)) : (a))

/*
extern ULONG  MajorVersion;
extern ULONG  MinorVersion;
extern ULONG  BuildNumber;
extern ULONG  SPVersion;
*/

#ifdef __cplusplus
};
#endif //__cplusplus

#ifndef USER_MODE
extern UNICODE_STRING SavedSPString;
#endif //USER_MODE

/*
#define WinVer_Is351  (MajorVersion==0x03)
#define WinVer_IsNT   (MajorVersion==0x04)
#define WinVer_Is2k   (MajorVersion==0x05 && MinorVersion==0x00)
#define WinVer_IsXP   (MajorVersion==0x05 && MinorVersion==0x01)
#define WinVer_IsdNET (MajorVersion==0x05 && MinorVersion==0x02)

#define WinVer_Id()   ((MajorVersion << 8) | MinorVersion)

#define WinVer_351    (0x0351)
#define WinVer_NT     (0x0400)
#define WinVer_2k     (0x0500)
#define WinVer_XP     (0x0501)
#define WinVer_dNET   (0x0502)
*/

#define PtrOffset(BASE,OFFSET) ((ULONG)((ULONG)(OFFSET) - (ULONG)(BASE)))

#ifndef offsetof
#define offsetof(type, field)   (ULONG)&(((type *)0)->field)
#endif //offsetof

#pragma pack(pop)

#endif // __TOOLS_H__
