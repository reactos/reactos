/*++

Copyright (c) 2002-2005 Alexandr A. Telyatnikov (Alter)

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

/*

#if defined _X86_ && !defined(__GNUC__)

#define MOV_DD_SWP(a,b)           \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm mov eax,[ebx]           \
    __asm bswap eax               \
    __asm mov ebx,_to_            \
    __asm mov [ebx],eax           \
}

#define MOV_DW_SWP(a,b)           \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm mov ax,[ebx]            \
    __asm rol ax,8                \
    __asm mov ebx,_to_            \
    __asm mov [ebx],ax            \
}

#define REVERSE_DD(a) {           \
    PFOUR_BYTE _from_;            \
    _from_ = ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm mov eax,[ebx]           \
    __asm bswap eax               \
    __asm mov [ebx],eax           \
}

#define REVERSE_DW(a) {           \
    PFOUR_BYTE _from_;            \
    _from_ = ((PFOUR_BYTE)&(a));  \
    __asm mov eax,_from_          \
    __asm rol word ptr [eax],8    \
}

#define MOV_DW2DD_SWP(a,b)        \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm mov ax,[ebx]            \
    __asm rol ax,8                \
    __asm mov ebx,_to_            \
    __asm mov [ebx+2],ax          \
    __asm mov [ebx],0             \
}

#define MOV_SWP_DW2DD(a,b)        \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm xor eax,eax             \
    __asm mov ax,[ebx]            \
    __asm rol ax,8                \
    __asm mov ebx,_to_            \
    __asm mov [ebx],eax           \
}

#define MOV_MSF(a,b)              \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm mov eax,[ebx]           \
    __asm mov ebx,_to_            \
    __asm mov [ebx],ax            \
    __asm shr eax,16              \
    __asm mov [ebx+2],al          \
}

#define MOV_MSF_SWP(a,b)          \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm mov eax,[ebx]           \
    __asm mov ebx,_to_            \
    __asm mov [ebx+2],al          \
    __asm bswap eax               \
    __asm shr eax,8               \
    __asm mov [ebx],ax            \
}

#define XCHG_DD(a,b)              \
{                                 \
    PULONG _from_, _to_;          \
    _from_ = ((PULONG)&(b));      \
    _to_ =   ((PULONG)&(a));      \
    __asm mov ebx,_from_          \
    __asm mov ecx,_to_            \
    __asm mov eax,[ebx]           \
    __asm xchg eax,[ecx]          \
    __asm mov [ebx],eax           \
}

#else   // NO X86 optimization , use generic C/C++

#define MOV_DD_SWP(a,b)           \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    _to_->Byte0 = _from_->Byte3;  \
    _to_->Byte1 = _from_->Byte2;  \
    _to_->Byte2 = _from_->Byte1;  \
    _to_->Byte3 = _from_->Byte0;  \
}

#define MOV_DW_SWP(a,b)           \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    _to_->Byte0 = _from_->Byte1;  \
    _to_->Byte1 = _from_->Byte0;  \
}

#define REVERSE_DD(a) {           \
    ULONG _i_;                    \
    MOV_DD_SWP(_i_,(a));          \
    *((PULONG)&(a)) = _i_;        \
}

#define REVERSE_DW(a) {           \
    USHORT _i_;                   \
    MOV_DW_SWP(_i_,(a));          \
    *((PUSHORT)&(a)) = _i_;       \
}

#define MOV_DW2DD_SWP(a,b)        \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    *((PUSHORT)_to_) = 0;         \
    _to_->Byte2 = _from_->Byte1;  \
    _to_->Byte3 = _from_->Byte0;  \
}

#define MOV_MSF(a,b)              \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    _to_->Byte0 = _from_->Byte0;  \
    _to_->Byte1 = _from_->Byte1;  \
    _to_->Byte2 = _from_->Byte2;  \
}

#define MOV_MSF_SWP(a,b)          \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    _to_->Byte0 = _from_->Byte2;  \
    _to_->Byte1 = _from_->Byte1;  \
    _to_->Byte2 = _from_->Byte0;  \
}

#define XCHG_DD(a,b)              \
{                                 \
    ULONG  _temp_;                \
    PULONG _from_, _to_;          \
    _from_ = ((PULONG)&(b));      \
    _to_ =   ((PULONG)&(a));      \
    _temp_ = *_from_;             \
    *_from_ = *_to_;              \
    *_to_ = _temp_;               \
}

#endif // _X86_

#define MOV_3B_SWP(a,b)     MOV_MSF_SWP(a,b)

*/

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

#define BrutePoint() {}

#define DbgAllocatePool(x,y) ExAllocatePool(x,y)
#define DbgFreePool(x) ExFreePool(x)
#define DbgAllocatePoolWithTag(a,b,c) ExAllocatePoolWithTag(a,b,c)

#else

#define KdDump(a,b) {}

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

#endif // __TOOLS_H__
