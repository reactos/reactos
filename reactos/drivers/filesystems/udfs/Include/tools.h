////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
#ifndef __TOOLS_H__
#define __TOOLS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __CROSSNT_MISC__H__

typedef struct _FOUR_BYTE {
    UCHAR Byte0;
    UCHAR Byte1;
    UCHAR Byte2;
    UCHAR Byte3;
} FOUR_BYTE, *PFOUR_BYTE;

#if defined _X86_ && defined __CROSS_VERSION_LIB_NT__H__

#define AcquireXLock(gLock, oldValue, newValue)       \
{                                                     \
    PULONG _gLock_ = &(gLock);                        \
    __asm push ebx                                    \
    __asm mov  eax,newValue                           \
    __asm mov  ebx,_gLock_                            \
    __asm xchg eax,[ebx]                              \
    __asm mov oldValue,eax                            \
    __asm pop  ebx                                    \
}

void
__fastcall
_MOV_DD_SWP(
    void* a, // ECX
    void* b  // EDX
    );
#define MOV_DD_SWP(a,b) _MOV_DD_SWP(&(a),&(b))
/*#define MOV_DD_SWP(a,b)           \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm mov eax,[ebx]           \
    __asm bswap eax               \
    __asm mov ebx,_to_            \
    __asm mov [ebx],eax           \
}*/

void
__fastcall
_MOV_DW_SWP(
    void* a, // ECX
    void* b  // EDX
    );
#define MOV_DW_SWP(a,b) _MOV_DW_SWP(&(a),&(b))
/*#define MOV_DW_SWP(a,b)           \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    __asm push ebx                \
    __asm mov ebx,_from_          \
    __asm mov ax,[ebx]            \
    __asm rol ax,8                \
    __asm mov ebx,_to_            \
    __asm mov [ebx],ax            \
    __asm pop ebx                 \
}*/

void
__fastcall
_REVERSE_DD(
    void* a  // ECX
    );
#define REVERSE_DD(a) _REVERSE_DD(&(a))
/*#define REVERSE_DD(a) {           \
    PFOUR_BYTE _from_;            \
    _from_ = ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm mov eax,[ebx]           \
    __asm bswap eax               \
    __asm mov [ebx],eax           \
}*/

void
__fastcall
_REVERSE_DW(
    void* a  // ECX
    );
#define REVERSE_DW(a) _REVERSE_DW(&(a))
/*#define REVERSE_DW(a) {           \
    PFOUR_BYTE _from_;            \
    _from_ = ((PFOUR_BYTE)&(a));  \
    __asm mov eax,_from_          \
    __asm rol word ptr [eax],8    \
}*/

void
__fastcall
_MOV_DW2DD_SWP(
    void* a, // ECX
    void* b  // EDX
    );
#define MOV_DW2DD_SWP(a,b) _MOV_DW2DD_SWP(&(a),&(b))
/*#define MOV_DW2DD_SWP(a,b)        \
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
}*/

void
__fastcall
_MOV_MSF(
    void* a, // ECX
    void* b  // EDX
    );
#define MOV_MSF(a,b) _MOV_MSF(&(a),&(b))
/*#define MOV_MSF(a,b)              \
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
}*/

void
__fastcall
_MOV_MSF_SWP(
    void* a, // ECX
    void* b  // EDX
    );
#define MOV_MSF_SWP(a,b) _MOV_MSF_SWP(&(a),&(b))
/*#define MOV_MSF_SWP(a,b)          \
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
}*/

void
__fastcall
_XCHG_DD(
    void* a, // ECX
    void* b  // EDX
    );
#define XCHG_DD(a,b) _XCHG_DD(&(a),&(b))
/*#define XCHG_DD(a,b)              \
{                                 \
    PULONG _from_, _to_;          \
    _from_ = ((PULONG)&(b));      \
    _to_ =   ((PULONG)&(a));      \
    __asm mov ebx,_from_          \
    __asm mov ecx,_to_            \
    __asm mov eax,[ebx]           \
    __asm xchg eax,[ecx]          \
    __asm mov [ebx],eax           \
}*/

#else   // NO X86 optimization , use generic C/C++

#define AcquireXLock(gLock, oldValue, newValue)      \
{                                                    \
    oldValue = gLock;                                \
    gLock = newValue;                                \
}

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

#endif // __CROSSNT_MISC__H__

#define CONV_TO_LL(a) a.Byte0 | a.Byte1 << 8 | a.Byte2 << 16 | a.Byte3 << 8
#define MSF_TO_LBA(Minutes,Seconds,Frames) \
                (ULONG)((60 * 75 * (Minutes)) + (75 * (Seconds)) + ((Frames) - 150))

#define PacketFixed2Variable(x,ps) ( ( ( (x) / (ps) )   * (ps+7) ) + ( (x) & (ps-1) ) )
#define PacketVariable2Fixed(x,ps) ( ( ( (x) / (ps+7) ) * (ps)   ) + ( (((x) % (ps+7)) < (ps)) ? ((x) % (ps+7)) : (ps-1) ) )

#define WAIT_FOR_XXX_EMU_DELAY  1000LL        //  0.0001 s

#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))

#ifndef offsetof
#define offsetof(type, field)   (ULONG)&(((type *)0)->field)
#endif //offsetof

#ifdef __cplusplus
};
#endif

#endif // __TOOLS_H__
