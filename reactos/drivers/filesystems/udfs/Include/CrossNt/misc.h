#ifndef __CROSSNT_MISC__H__
#define __CROSSNT_MISC__H__

#ifdef _X86_

typedef void
(__fastcall *ptrMOV_DD_SWP)(
    void* a, // ECX
    void* b  // EDX
    );
extern "C" ptrMOV_DD_SWP _MOV_DD_SWP;

extern "C"
void
__fastcall
_MOV_DD_SWP_i486(
    void* a, // ECX
    void* b  // EDX
    );

extern "C"
void
__fastcall
_MOV_DD_SWP_i386(
    void* a, // ECX
    void* b  // EDX
    );
#define MOV_DD_SWP(a,b) _MOV_DD_SWP(&(a),&(b))

/********************/

extern "C"
void
__fastcall
_MOV_DW_SWP(
    void* a, // ECX
    void* b  // EDX
    );

#define MOV_DW_SWP(a,b) _MOV_DW_SWP(&(a),&(b))

/********************/

typedef void
(__fastcall *ptrREVERSE_DD)(
    void* a  // ECX
    );
extern "C" ptrREVERSE_DD _REVERSE_DD;

void
__fastcall
_REVERSE_DD_i486(
    void* a  // ECX
    );

void
__fastcall
_REVERSE_DD_i386(
    void* a  // ECX
    );
#define REVERSE_DD(a) _REVERSE_DD(&(a))

/********************/

extern "C"
void
__fastcall
_REVERSE_DW(
    void* a  // ECX
    );

#define REVERSE_DW(a) _REVERSE_DW(&(a))

/********************/

extern "C"
void
__fastcall
_MOV_DW2DD_SWP(
    void* a, // ECX
    void* b  // EDX
    );

#define MOV_DW2DD_SWP(a,b) _MOV_DW2DD_SWP(&(a),&(b))

/********************/

extern "C"
void
__fastcall
_MOV_SWP_DW2DD(
    void* a, // ECX
    void* b  // EDX
    );

#define MOV_SWP_DW2DD(a,b) _MOV_SWP_DW2DD(&(a),&(b))

/********************/

extern "C"
void
__fastcall
_MOV_MSF(
    void* a, // ECX
    void* b  // EDX
    );
#define MOV_MSF(a,b) _MOV_MSF(&(a),&(b))

/********************/

typedef void
(__fastcall *ptrMOV_MSF_SWP)(
    void* a, // ECX
    void* b  // EDX
    );
extern "C" ptrMOV_MSF_SWP _MOV_MSF_SWP;

extern "C"
void
__fastcall
_MOV_MSF_SWP_i486(
    void* a, // ECX
    void* b  // EDX
    );

extern "C"
void
__fastcall
_MOV_MSF_SWP_i386(
    void* a, // ECX
    void* b  // EDX
    );
#define MOV_MSF_SWP(a,b) _MOV_MSF_SWP(&(a),&(b))

/********************/

extern "C"
void
__fastcall
_XCHG_DD(
    void* a, // ECX
    void* b  // EDX
    );
#define XCHG_DD(a,b) _XCHG_DD(&(a),&(b))

#endif //_X86_

#endif // __CROSSNT_MISC__H__
